#include "transport.h"
#include "rpc-lib/pipe/ipipe.h"
#include <cassert>

namespace vshalygin::rpc {
    transport::transport(std::shared_ptr<ipipe> pipe)
        : pipe_(std::move(pipe))
    {
        assert(pipe_ && pipe_->is_connected());
    }

    transport::~transport()
    {
        stop();
    }

    void transport::send_async(cl::buffer &&message,
                                    std::function<void()> &&error_handler)
    {
        auto res = pipe_->write_async(std::move(message),
                                     [eh = std::move(error_handler)](pipe_op_res res) {
                                         if(res == pipe_op_res::failed) {
                                             eh();
                                         }
                                     });

        if(!res) {
            throw std::runtime_error("write_async failed");
        }
    }

    void transport::recv_async(std::function<void(cl::buffer &&)> &&handler)
    {
        auto r = pipe_->read_async([handler = std::move(handler), this](pipe_op_res res, cl::buffer &&msg) {
                                       if(is_success(res)) {
                                           handler(std::move(msg));
                                       } else {
                                           stop();
                                       }
                                   });

        if(!r) {
            stop();
        }
    }

    void transport::start(std::function<void()> &&start_callback, std::function<void()> &&stop_callback)
    {
        assert(start_callback);
        assert(stop_callback);

        std::lock_guard lock(mtx_);
        if(state_ != state::init) {
            throw std::logic_error("pipe transport was started");
        }

        auto res = pipe_->wait_connect_for(std::chrono::seconds(10));
        if(!res) {
            throw std::runtime_error("unable to wait pipe connection");
        }

        stop_callback_ = std::move(stop_callback);
        state_ = state::started;

        if(start_callback) try {
            start_callback();
        } catch (...) {
            //TODO log
        }
    }

    void transport::stop()
    {
        std::lock_guard lock(mtx_);
        if(state_ == state::started) {
            pipe_->invalidate();
            state_ = state::stopped;
            try {
                stop_callback_();
            } catch(...) {
                //TODO log
            }
        }
    }

    bool transport::is_running() const
    {
        std::lock_guard lock(mtx_);
        return state_ == state::started;
    }
}
