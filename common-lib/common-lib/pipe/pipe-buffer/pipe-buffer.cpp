#include "pipe-buffer.h"
#include <future>

namespace vshalygin::cl {
    std::shared_ptr<pipe_buffer> pipe_buffer::create(std::shared_ptr<thread_pool> thread_pool)
    {
        return std::make_shared<pipe_buffer>(std::move(thread_pool), creator{});
    }

    pipe_buffer::pipe_buffer(std::shared_ptr<thread_pool> thread_pool, creator)
        : strand_(thread_pool->create_strand())
    {}

    pipe_buffer::~pipe_buffer()
    {
        invalidate();
    }

    void pipe_buffer::write_async(std::string &&data, std::function<void(bool)> &&handler)
    {
        auto safe_handler = [handler = std::move(handler)](bool res) {
            try {
                handler(res);
            } catch (...) {
                //TODO log
            }
        };

        auto task = [self = weak_from_this(),
                     data = std::move(data),
                     write_handler = std::move(safe_handler)]() mutable {
            if(auto s = self.lock()) {
                try {
                    s->buffer_.push(std::move(data));
                } catch (...) {
                    write_handler(false);
                    throw;
                }
                write_handler(true);

                if(!s->read_handlers_.empty()) {
                    auto read_handler = std::move(s->read_handlers_.front());
                    s->read_handlers_.pop();
                    read_handler(true, std::move(data));
                }
            }
        };

        strand_.post(std::move(task));
    }

    void pipe_buffer::read_async(std::function<void(bool, std::string &&)> &&handler)
    {
        assert(handler);

        auto safe_handler = [handler = std::move(handler)](bool res, std::string &&str) {
            try {
                return handler(res, std::move(str));
            } catch (...) {
                //TODO log
            }
        };

        strand_.post([self = weak_from_this(), handler = std::move(safe_handler)]() mutable {
            if(auto s = self.lock()) {
                if(!s->is_valid_) {
                    handler(false, {});
                    return;
                }

                if(!s->buffer_.empty()) {
                    auto msg = std::move(s->buffer_.front());
                    s->buffer_.pop();
                    handler(true, std::move(msg));
                } else {
                    s->read_handlers_.push(std::move(handler));
                }
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
}
