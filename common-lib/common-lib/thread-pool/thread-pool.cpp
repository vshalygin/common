#include "thread-pool.h"
#include "strand.h"

#include <boost/asio/post.hpp>

namespace vshalygin::cl {
    thread_pool::thread_pool(unsigned thread_num)
        : m_thread_num(thread_num)
        , m_executor_work_guard(boost::asio::make_work_guard(m_io_context))
    {
        while(thread_num--) {
            m_thread_group.create_thread([this]() {
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

    void thread_pool::stop()
    {
        m_io_context.stop();

        std::call_once(m_join_threads_flag, [this]() { m_thread_group.join_all(); });
    }

    bool thread_pool::is_stopped() const
    {
        return m_io_context.stopped();
    }

    unsigned thread_pool::get_num() const noexcept
    {
        return m_thread_num;
    }

    boost::asio::io_context &thread_pool::get_io_context() noexcept
    {
        return m_io_context;
    }

    const boost::asio::io_context &thread_pool::get_io_context() const noexcept
    {
        return m_io_context;
    }

    strand thread_pool::create_strand()
    {
        return strand(m_io_context);
    }
}
