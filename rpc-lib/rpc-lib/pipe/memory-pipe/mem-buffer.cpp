#include "mem-buffer.h"
#include <future>

namespace vshalygin::rpc {
    mem_buffer::mem_buffer(std::shared_ptr<cl::thread_pool> thread_pool)
        : strand_(thread_pool->create_strand())
    {}

    mem_buffer::~mem_buffer()
    {
        invalidate();
    }

    void mem_buffer::write_async(cl::buffer &&data, std::function<void(pipe_op_res)> &&callback)
    {
        assert(callback);

        auto task = [this,
                     data = std::move(data),
                     callback = std::move(callback)]() mutable {
            if(!is_valid_) {
                callback(pipe_op_res::failed);
                return;
            }

            try {
                buffer_.push(std::move(data));
            } catch(...) {
                callback(pipe_op_res::failed);
                throw;
            }
            callback(pipe_op_res::success);

            if(!read_handlers_.empty()) {
                auto read_handler = std::move(read_handlers_.front());
                read_handlers_.pop();
                read_handler(pipe_op_res::success, std::move(buffer_.front()));
                buffer_.pop();
            }
        };

        strand_.post(std::move(task));
    }

    void mem_buffer::read_async(std::function<void(pipe_op_res, cl::buffer &&)> &&callback)
    {
        assert(callback);

        strand_.post([this, callback = std::move(callback)]() mutable {
            if(!is_valid_) {
                callback(pipe_op_res::failed, {});
                return;
            }

            if(!buffer_.empty()) {
                auto msg = std::move(buffer_.front());
                buffer_.pop();
                callback(pipe_op_res::success, std::move(msg));
            } else {
                try {
                    read_handlers_.push(std::move(callback));
                } catch(...) {
                    callback(pipe_op_res::failed, {});
                    throw;
                }
            }
        });
    }

    void mem_buffer::invalidate() noexcept
    {
        try {
            std::packaged_task<void()> task([this]() {
                if(is_valid_) {
                    is_valid_ = false;
                    buffer_ = {};
                    while(!read_handlers_.empty()) {
                        read_handlers_.front()(pipe_op_res::canceled, {});
                        read_handlers_.pop();
                    }
                }
            });
            auto f = task.get_future();

            strand_.dispatch(std::move(task));

            f.get();
        } catch(...) {
            //TODO log
        }
    }

    bool mem_buffer::is_valid() const
    {
        std::packaged_task<bool()> task([this]() {
            return is_valid_;
        });
        auto f = task.get_future();

        strand_.dispatch(std::move(task));

        return f.get();
    }

    size_t mem_buffer::get_pending_messages_count() const
    {
        std::packaged_task<bool()> task([this]() {
            return buffer_.size();
        });
        auto f = task.get_future();

        strand_.dispatch(std::move(task));

        return f.get();
    }

    size_t mem_buffer::get_pending_read_handlers_count() const
    {
        std::packaged_task<bool()> task([this]() {
            return read_handlers_.size();
        });
        auto f = task.get_future();

        strand_.dispatch(std::move(task));

        return f.get();
    }
}
