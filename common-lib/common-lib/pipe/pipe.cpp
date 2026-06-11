#include "pipe.h"
#include "common-lib/syncronization/event/event.h"

#include <mutex>
#include <condition_variable>

namespace vshalygin::cl {
    pipe::pipe(bool is_server)
        : is_server_(is_server)
    {}

    pipe::~pipe()
    {
        try {
            invalidate();
        } catch(...) {
            //TODO log fatal
            std::terminate();
        }
    }

    void pipe::set_buffers(std::shared_ptr<pipe_buffers> pipe_buffers)
    {
        {
            std::lock_guard guard(mtx_);
            pipe_buffers_ = std::move(pipe_buffers);
        }

        cv_.notify_all();
    }

    bool pipe::is_connected() const
    {
        std::lock_guard guard(mtx_);
        return pipe_buffers_ &&
               pipe_buffers_->client_to_server.is_valid() &&
               pipe_buffers_->server_to_client.is_valid();
    }

    bool pipe::wait_connect_for(const std::chrono::microseconds &mcs) const
    {
        auto now = std::chrono::steady_clock::now();
        std::unique_lock lock(mtx_);
        return cv_.wait_until(lock, now + mcs,
                              [this]() {
                                  return stop_flag_ || pipe_buffers_ != nullptr;
                              });
    }

    bool pipe::wait_connect() const
    {
        std::unique_lock lock(mtx_);
        cv_.wait(lock,[this]() {
                          return stop_flag_ || pipe_buffers_ != nullptr;
                      });

        return !stop_flag_;
    }

    bool pipe::write_async(buffer &&msg, std::function<void(pipe_op_res)> &&handler)
    {
        std::lock_guard guard(mtx_);
        if(!pipe_buffers_) {
            return false;
        }
        auto &output_buff = is_server_ ?
                            pipe_buffers_->server_to_client :
                            pipe_buffers_->client_to_server;
        if(!output_buff.is_valid()) {
            return false;
        }

        output_buff.write_async(std::move(msg), std::move(handler));
        return true;
    }

    bool pipe::read_async(std::function<void(pipe_op_res, buffer &&)> &&handler)
    {
        std::lock_guard guard(mtx_);
        if(!pipe_buffers_) {
            return false;
        }
        auto &input_buff = is_server_ ?
                           pipe_buffers_->client_to_server :
                           pipe_buffers_->server_to_client;

        if(!input_buff.is_valid()) {
            return false;
        }

        input_buff.read_async(std::move(handler));
        return true;
    }

    bool pipe::try_to_write_for(buffer &&msg, const std::chrono::microseconds &timeout)
    {
        struct data
        {
            event sync_event;
            pipe_op_res res = pipe_op_res::failed;
        };
        auto d = std::make_shared<data>();
        auto task = [d](pipe_op_res r) {
            d->res = r;
            d->sync_event.set();
        };

        if(!write_async(std::move(msg), std::move(task))) {
            return false;
        }

        if(!d->sync_event.wait_for(timeout)) {
            return false;
        }

        return is_success(d->res);
    }

    std::optional<buffer> pipe::try_to_read_for(const std::chrono::microseconds &timeout)
    {
        struct data
        {
            event sync_event;
            pipe_op_res res = pipe_op_res::failed;
            buffer buf;
        };
        auto d = std::make_shared<data>();
        auto task = [d](pipe_op_res r, buffer &&b) {
            d->res = r;
            d->buf = std::move(b);
            d->sync_event.set();
        };

        if(!read_async(std::move(task))) {
            return std::nullopt;
        }

        if(!d->sync_event.wait_for(timeout)) {
            return std::nullopt;
        }

        return is_success(d->res) ? std::optional<buffer>(std::move(d->buf)) : std::nullopt;
    }

    void pipe::invalidate()
    {
        std::unique_lock lock(mtx_);
        stop_flag_ = true;

        lock.unlock();
        cv_.notify_all();
        lock.lock();

        if(pipe_buffers_) {
            pipe_buffers_->client_to_server.invalidate();
            pipe_buffers_->server_to_client.invalidate();
        }
    }
}
