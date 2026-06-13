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
    namespace {
        std::string make_unique_pipe_name()
        {
            static std::atomic_uint64_t counter = 0;
            return std::to_string(counter++);
        }
    }

    listener::listener(std::shared_ptr<ipipe_env> pipe_env,
                       std::shared_ptr<iauthenticator> authenticator)
        : pipe_env_(std::move(pipe_env))
        , authenticator_(std::move(authenticator))
    {
        assert(pipe_env_);
        assert(authenticator_);
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

        auto con_msg = pipe->try_to_read_for(std::chrono::seconds(10));
        if(!con_msg) {
            return;
        }
        proto::auth_request req;
        if(!req.ParseFromArray(con_msg->data(), static_cast<int>(con_msg->size()))) {
            return;
        }

        proto::auth_response res;
        if(authenticator_->check_request(req)) {
            res.set_is_accepted(true);
            res.set_pipe_data(make_unique_pipe_name());
        } else {
            res.set_is_accepted(false);
        }


        cl::buffer buf(res.ByteSizeLong());
        if(!res.SerializeToArray(buf.data(), static_cast<int>(buf.size()))) {
            return;
        }

        if(!pipe->try_to_write_for(std::move(buf), std::chrono::seconds(10))) {
            return;
        }
        
        if(res.is_accepted()) {
            connect_handler_(std::make_unique<transport>(std::move(pipe)));
        } else {
            pipe->invalidate(); //TODO what if invalidation before reading?
        }
    }
}
