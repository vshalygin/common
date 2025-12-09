#include "thread-pool.h"
#include "strand.h"

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

    std::unique_ptr<strand> thread_pool::create_strand()
    {
        return std::make_unique<strand>(m_io_context);
    }

}
