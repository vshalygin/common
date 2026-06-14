#include "listener.h"

#pragma warning(push, 0)
#include "rpc-lib/authenticator/proto/auth.pb.h"
#pragma warning(pop)

#include <rpc-lib/transport/transport.h>
#include "rpc-lib/authenticator/iauthenticator.h"
#include <rpc-lib/pipe/ipipe.h>
#include <rpc-lib/pipe/ipipe-env.h>
#include <common-lib/utils/buffer/buffer.h>

#include <cassert>
#include <atomic>
#include <string>

namespace vshalygin::rpc {
    listener::listener(std::shared_ptr<ipipe_env> pipe_env,
                       std::shared_ptr<iauthenticator> authenticator)
        : pipe_env_(std::move(pipe_env))
        , authenticator_(std::move(authenticator))
    {
        assert(pipe_env_);
        assert(authenticator_);
    }

    listener::~listener()
    {
        try {
            stop();
        } catch(...) {
            //TODO log
            std::terminate();
        }
    }

    void listener::start()
    {
        assert(connect_handler_);

        std::lock_guard guard(mtx_);
        if(is_running_) {
            throw std::runtime_error("pipe listener already started");
        }
        
        listen_thread_ = std::jthread([this](std::stop_token st) {
            while(!st.stop_requested()) {
                try {
                    create_new_connection();
                } catch(...) {
                    //TODO log
                }
            }
        });

        is_running_ = true;
        auto [g, state_change_handler] = state_change_handler_.get();
        if(state_change_handler) {
            state_change_handler(listener_state::started);
        }
    }

    void listener::stop()
    {
        std::lock_guard guard(mtx_);
        if(is_running_) {
            listen_thread_.request_stop();
            if(current_pipe_) {
                current_pipe_->invalidate();
                current_pipe_.reset();
            }
            listen_thread_.join();
            is_running_ = false;

            auto [g, state_change_handler] = state_change_handler_.get();
            if(state_change_handler) {
                state_change_handler(listener_state::stopped);
            }
        }
    }

    bool listener::is_stopped() const
    {
        std::lock_guard guard(mtx_);
        return !is_running_;
    }

    void listener::set_change_state_handler(change_state_handler_t &&handler)
    {
        auto [guard, state_change_handler] = state_change_handler_.get();
        state_change_handler = std::move(handler);
    }

    void listener::set_connect_handler(connect_handler_t &&handler)
    {
        assert(!connect_handler_);

        connect_handler_ = std::move(handler);
    }

    void listener::create_new_connection()
    {
        std::unique_lock guard(mtx_);
        if(!is_running_) {
            return;
        }
        auto pipe = pipe_env_->create_pipe();

        current_pipe_ = pipe;
        guard.unlock();

        if(!pipe->wait_connect()) {
            return;
        }

        try {
            auto con_msg = pipe->try_to_read_for(std::chrono::seconds(10));
            if(!con_msg) {
                throw std::runtime_error("failed to read auth request");;
            }
            proto::auth_request req;
            if(!req.ParseFromArray(con_msg->data(), static_cast<int>(con_msg->size()))) {
                throw std::runtime_error("failed to parse auth request");
            }

            proto::auth_response res;
            res.set_is_accepted(authenticator_->check_request(req));

            cl::buffer buf(res.ByteSizeLong());
            if(!res.SerializeToArray(buf.data(), static_cast<int>(buf.size()))) {
                throw std::runtime_error("failed to serialize auth response");
            }

            if(!pipe->try_to_write_for(std::move(buf), std::chrono::seconds(10))) {
                throw std::runtime_error("failed to write auth response");
            }

            if(res.is_accepted()) {
                connect_handler_(std::make_unique<transport>(std::move(pipe)));
            } else {
                pipe->invalidate();
            }
        } catch (...) {
            pipe->invalidate();
            throw;
        }
    }
}
