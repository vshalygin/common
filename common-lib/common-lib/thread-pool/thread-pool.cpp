#include "thread-pool.h"
#include <boost/asio/post.hpp>

namespace vsh::cl {
    thread_pool::thread_pool(unsigned thread_num)
        : m_thread_num(thread_num)
        , m_executor_work_guard(boost::asio::make_work_guard(m_io_context))
    {
        while(thread_num--) {
            thread_group_.create_thread([this]() {
                                            while(!m_io_context.stopped()) {
                                                try {
                                                    m_io_context.run();
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
        m_io_context.stop();

        std::call_once(join_threads_flag_, [this]() { thread_group_.join_all(); });
    }

    bool thread_pool::is_stopped() const
    {
        return m_io_context.stopped();
    }

    unsigned thread_pool::get_num() const
    {
        return m_thread_num;
    }

    boost::asio::io_context *thread_pool::get_io_context()
    {
        return &m_io_context;
    }

    const boost::asio::io_context *thread_pool::get_io_context() const
    {
        return &m_io_context;
    }

    template<typename Func>
    void thread_pool::post_impl(Func &&func) const
    {
        boost::asio::post(std::forward<Func>(func));
    }
}
