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
        try {
            std::promise<void> p;
            auto f = p.get_future();

            strand_.post([&] { p.set_value(); });

            f.wait();
        } catch (...) {
            //TODO log
        }
    }

    void pipe_buffer::write_async(std::string &&data)
    {
        auto task = [self = weak_from_this(), data = std::move(data)]() {
            if(auto s = self.lock()) {
                s->buffer_.push(std::move(data));
                s->read_async();
            }
        };

        strand_.post(std::move(task));
    }

    void pipe_buffer::start_reading_async(std::function<void(std::string &&)> &&handler)
    {
        assert(handler);
        assert(!handler_); //sets one once by design

        auto safe_handler = [handler = std::move(handler)](std::string &&str) {
            try {
                handler(std::move(str));
            } catch (...) {
                //TODO log
            }
        };

        strand_.post([self = weak_from_this(), handler = std::move(safe_handler)]() {
            if(auto s = self.lock()) {
                s->handler_ = std::move(handler);
                s->read_async();
            }
        });;
    }

    void pipe_buffer::read_async()
    {
        auto task = [self = weak_from_this()]() {
            if(auto s = self.lock(); s && s->handler_) {
                while(!s->buffer_.empty()) {
                    auto msg = std::move(s->buffer_.front());
                    s->buffer_.pop();
                    s->handler_(std::move(msg));
                }
            }
        };

        strand_.post(std::move(task));
    }
}
