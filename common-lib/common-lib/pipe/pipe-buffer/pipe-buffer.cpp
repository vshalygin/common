#include "pipe-buffer.h"
#include <future>

namespace vshalygin::cl {
    pipe_buffer::pipe_buffer(std::shared_ptr<thread_pool> thread_pool)
        : strand_(thread_pool->create_strand())
    {}

    pipe_buffer::~pipe_buffer()
    {
        invalidate();
    }

    void pipe_buffer::write_async(buffer &&data, std::function<void(bool)> &&handler)
    {
        auto safe_handler = [handler = std::move(handler)](bool res) {
            try {
                if(handler) {
                    handler(res);
                }
            } catch (...) {
                //TODO log
            }
        };

        auto task = [this,
                     data = std::move(data),
                     write_handler = std::move(safe_handler)]() mutable {
            try {
                buffer_.push(std::move(data));
            } catch(...) {
                write_handler(false);
                throw;
            }
            write_handler(true);

            if(!read_handlers_.empty()) {
                auto read_handler = std::move(read_handlers_.front());
                read_handlers_.pop();
                read_handler(true, std::move(buffer_.front()));
                buffer_.pop();
            }
        };

        strand_.post(std::move(task));
    }

    void pipe_buffer::read_async(std::function<void(bool, buffer &&)> &&handler)
    {
        auto safe_handler = [handler = std::move(handler)](bool res, buffer &&str) {
            try {
                if(handler) {
                    handler(res, std::move(str));
                }
            } catch (...) {
                //TODO write in log
            }
        };

        strand_.post([this, handler = std::move(safe_handler)]() mutable {
            if(!is_valid_) {
                handler(false, {});
                return;
            }

            if(!buffer_.empty()) {
                auto msg = std::move(buffer_.front());
                buffer_.pop();
                handler(true, std::move(msg));
            } else {
                read_handlers_.push(std::move(handler));
            }
        });
    }

    void pipe_buffer::invalidate() noexcept
    {
        try {
            std::packaged_task<void()> task([this]() {
                is_valid_ = false;
                while(!read_handlers_.empty()) {
                    read_handlers_.front()(false, {});
                    read_handlers_.pop();
                }
            });
            auto f = task.get_future();

            strand_.dispatch(std::move(task));

            f.get();
        } catch(...) {
            //TODO log
        }
    }

    bool pipe_buffer::is_valid() const
    {
        std::packaged_task<bool()> task([this]() {
            return is_valid_;
        });
        auto f = task.get_future();

        strand_.dispatch(std::move(task));

        return f.get();
    }

    size_t pipe_buffer::get_pending_messages_count() const
    {
        std::packaged_task<bool()> task([this]() {
            return buffer_.size();
        });
        auto f = task.get_future();

        strand_.dispatch(std::move(task));

        return f.get();
    }

    size_t pipe_buffer::get_pending_read_handlers_count() const
    {
        std::packaged_task<bool()> task([this]() {
            return read_handlers_.size();
        });
        auto f = task.get_future();

        strand_.dispatch(std::move(task));

        return f.get();
    }
}
