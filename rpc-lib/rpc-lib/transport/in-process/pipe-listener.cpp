#include "pipe-listener.h"
#include "pipe-transport.h"

#pragma warning(push, 0)
#include "rpc-lib/transport/proto/pipe-auth.pb.h"
#pragma warning(pop)

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

        bool check_auth(const proto::pipe_auth_request & /*data*/)
        {
            return true; //well, allow anyone
        }
    }

    pipe_listener::pipe_listener(std::shared_ptr<ipipe_env> pipe_env,
                                 const std::string &listener_pipe_name)
        : pipe_env_(std::move(pipe_env))
        , listener_pipe_name_(listener_pipe_name)
    {
        assert(pipe_env_);
    }

    void pipe_listener::start()
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

    void pipe_listener::stop()
    {
        std::lock_guard guard(mtx_);
        if(is_running_) {
            listen_thread_.request_stop();
            if(current_listener_pipe_) {
                current_listener_pipe_->invalidate();
                current_listener_pipe_.reset();
            }
            listen_thread_.join();
            is_running_ = false;

            auto [g, state_change_handler] = state_change_handler_.get();
            if(state_change_handler) {
                state_change_handler(listener_state::stopped);
            }
        }
    }

    bool pipe_listener::is_stopped() const
    {
        std::lock_guard guard(mtx_);
        return !is_running_;
    }

    void pipe_listener::set_change_state_handler(change_state_handler_t &&handler)
    {
        auto [guard, state_change_handler] = state_change_handler_.get();
        state_change_handler = std::move(handler);
    }

    void pipe_listener::set_connect_handler(connect_handler_t &&handler)
    {
        assert(!connect_handler_);

        connect_handler_ = std::move(handler);
    }

    void pipe_listener::create_new_connection()
    {
        std::unique_lock guard(mtx_);
        if(!is_running_) {
            return;
        }
        auto listener_pipe = pipe_env_->create_pipe(listener_pipe_name_);

        current_listener_pipe_ = listener_pipe;
        guard.unlock();

        if(!listener_pipe->wait_connect()) {
            return;
        }

        auto con_msg = listener_pipe->try_to_read_for(std::chrono::seconds(10));
        if(!con_msg) {
            return;
        }
        proto::pipe_auth_request req;
        if(!req.ParseFromArray(con_msg->data(), static_cast<int>(con_msg->size()))) {
            return;
        }

        proto::pipe_auth_response res;
        if(check_auth(req)) {
            res.set_is_accepted(true);
            res.set_pipe_name(make_unique_pipe_name());
        } else {
            res.set_is_accepted(false);
        }


        cl::buffer buf(res.ByteSizeLong());
        if(!res.SerializeToArray(buf.data(), static_cast<int>(buf.size()))) {
            return;
        }

        if(!listener_pipe->try_to_write_for(std::move(buf), std::chrono::seconds(10))) {
            return;
        }
        
        if(res.is_accepted()) {
            auto pipe = pipe_env_->create_pipe(res.pipe_name());
            if(!pipe->wait_connect_for(std::chrono::seconds(10))) {
                return;
            }

            connect_handler_(std::make_unique<pipe_transport>(std::move(pipe)));
        }
    }
}
