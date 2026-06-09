#include "pipe-transport.h"

namespace vshalygin::rpc {
    pipe_transport::pipe_transport(std::shared_ptr<cl::pipe> pipe)
        : pipe_(std::move(pipe))
    {
        assert(pipe_);
    }

    void pipe_transport::send_async(cl::buffer &&message,
                                    std::function<void()> &&error_handler) const
    {
        auto res = pipe_->write_async(std::move(message),
                                     [eh = std::move(error_handler)](bool is_success) {
                                         if(!is_success) {
                                             eh();
                                         }
                                     });

        if(!res) {
            throw std::runtime_error("write_async failed");
        }
    }

    void pipe_transport::recv_async(std::function<void(cl::buffer &&)> &&handler) const
    {
        pipe_->read_async([handler = std::move(handler)](bool res, cl::buffer &&msg) {
                              if(res) {
                                  handler(std::move(msg));
                              }
                          });
    }

    void pipe_transport::start()
    {
        assert(start_callback_);
        assert(stop_callback_);
        
        std::unique_lock lock(start_mtx_);
        if(is_started_) {
            throw std::logic_error("pipe transport already started");
        }

        auto res = pipe_->wait_connect_for(std::chrono::seconds(10));
        if(!res) {
            throw std::runtime_error("unable to wait pipe connection");
        }

        is_started_ = true;
        lock.unlock();

        start_callback_();
    }

    void pipe_transport::stop()
    {
        assert(stop_callback_);

        pipe_->disconnect();
        try {
            stop_callback_();
        } catch(...) {
            //TODO log
        }
    }

    bool pipe_transport::is_stopped() const
    {
        return pipe_->is_connected();
    }

    void pipe_transport::set_start_callback(std::function<void()> &&callback)
    {
        start_callback_ = std::move(callback);
    }

    void pipe_transport::set_stop_callback(std::function<void()> &&callback)
    {
        stop_callback_ = std::move(callback);
    }
}
