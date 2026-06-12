#include "pipe-transport.h"

namespace vshalygin::rpc {
    pipe_transport::pipe_transport(std::shared_ptr<cl::pipe> pipe)
        : pipe_(std::move(pipe))
    {
        assert(pipe_ && pipe_->is_connected());
    }

    void pipe_transport::send_async(cl::buffer &&message,
                                    std::function<void()> &&error_handler) const
    {
        auto res = pipe_->write_async(std::move(message),
                                     [eh = std::move(error_handler)](cl::pipe_op_res res) {
                                         if(res == cl::pipe_op_res::failed) {
                                             eh();
                                         }
                                     });

        if(!res) {
            throw std::runtime_error("write_async failed");
        }
    }

    void pipe_transport::recv_async(std::function<void(cl::buffer &&)> &&handler) const
    {
        pipe_->read_async([handler = std::move(handler)](cl::pipe_op_res res, cl::buffer &&msg) {
                              if(is_success(res)) {
                                  handler(std::move(msg));
                              }
                          });
    }

    void pipe_transport::start(std::function<void()> &&start_callback, std::function<void()> &&stop_callback)
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

        start_callback();
    }

    void pipe_transport::stop()
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

    bool pipe_transport::is_running() const
    {
        std::lock_guard lock(mtx_);
        return state_ == state::started;
    }
}
