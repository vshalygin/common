#include "thread-pool.h"
#include <boost/asio/post.hpp>

namespace vsh::cl {
    thread_pool::thread_pool(unsigned thread_num)
        : thread_num_(thread_num)
        , executor_work_guard_(boost::asio::make_work_guard(io_context_))
    {
        while(thread_num--) {
            thread_group_.create_thread([this]() {
                                            while(!io_context_.stopped()) {
                                                try {
                                                    io_context_.run();
                                                    break;
                                                }catch(...) {
                                                }
                                            }
                                        });
        }
    }

    thread_pool::~thread_pool()
    {
        try {
            stop();
        } catch(...) {
        }
    }

    void thread_pool::post(std::function<void()> &&func) const
    {
        post_impl(std::move(func));
    }

    void thread_pool::post(const std::function<void()> &func) const
    {
        post_impl(func);
    }

    void thread_pool::stop()
    {
        io_context_.stop();

        std::call_once(join_threads_flag_, [this]() { thread_group_.join_all(); });
    }

    bool thread_pool::is_stopped() const
    {
        return io_context_.stopped();
    }

    unsigned thread_pool::get_num() const
    {
        return thread_num_;
    }

    boost::asio::io_context *thread_pool::get_io_context()
    {
        return &io_context_;
    }

    const boost::asio::io_context *thread_pool::get_io_context() const
    {
        return &io_context_;
    }

    template<typename Func>
    void thread_pool::post_impl(Func &&func) const
    {
        boost::asio::post(std::forward<Func>(func));
    }
}
