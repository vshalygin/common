#include "pipe.h"
#include <mutex>
#include <condition_variable>

namespace vshalygin::cl {
    pipe::pipe(bool is_server)
        : is_server_(is_server)
    {}

    pipe::~pipe()
    {
        try {
            disconnect();
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
                              [this, stop_waiting_flag = stop_waiting_flag_]() {
                                  return *stop_waiting_flag || pipe_buffers_ != nullptr;
                              });
    }

    void pipe::wait_connect() const
    {
        std::unique_lock lock(mtx_);
        cv_.wait(lock,[this, stop_waiting_flag = stop_waiting_flag_]() {
                          return *stop_waiting_flag || pipe_buffers_ != nullptr;
                      });
    }

    void pipe::stop_waiting_all() const
    {
        auto new_stop_waiting_flag = std::make_shared<bool>(false);

        {
            std::lock_guard lock(mtx_);
            *stop_waiting_flag_ = true;
            stop_waiting_flag_ = std::move(new_stop_waiting_flag);
        }

        cv_.notify_all();
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

    void pipe::disconnect()
    {
        std::lock_guard guard(mtx_);
        if(pipe_buffers_) {
            pipe_buffers_->client_to_server.invalidate();
            pipe_buffers_->server_to_client.invalidate();
        }
    }
}
