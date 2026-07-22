#pragma once
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/thread/thread.hpp>

namespace vshalygin::cl {
    struct thread_pool_default_tag
    {};

    template<typename Tag>
    class basic_thread_pool final
    {
    public:
        explicit basic_thread_pool(unsigned thread_num);
        ~basic_thread_pool();

        basic_thread_pool(const basic_thread_pool &) = delete;
        basic_thread_pool &operator=(const basic_thread_pool &) = delete;

        template<typename Task>
        void post(Task &&task) const;

        void stop();
        bool is_stopped() const;

        unsigned get_num() const noexcept;

        boost::asio::io_context &get_io_context() noexcept;
        const boost::asio::io_context &get_io_context() const noexcept;

    private:
        using io_context = boost::asio::io_context;
        using executor_work_guard = boost::asio::executor_work_guard<io_context::executor_type>;

        const unsigned m_thread_num;
        mutable io_context m_io_context;
        executor_work_guard m_executor_work_guard;

        std::once_flag m_join_threads_flag;
        boost::thread_group m_thread_group;
    };

    template<typename Tag>
    basic_thread_pool<Tag>::basic_thread_pool(unsigned thread_num)
        : m_thread_num(thread_num)
        , m_executor_work_guard(boost::asio::make_work_guard(m_io_context))
    {
        while(thread_num--) {
            m_thread_group.create_thread([this]() {
                while(!m_io_context.stopped()) {
                    try {
                        m_io_context.run();
                        break;
                    }
                    catch(...) {
                    }
                }
            });
        }
    }

    template<typename Tag>
    basic_thread_pool<Tag>::~basic_thread_pool()
    {
        stop();
    }

    template<typename Tag>
    template<typename Task>
    void basic_thread_pool<Tag>::post(Task &&task) const
    {
        boost::asio::post(m_io_context, std::forward<Task>(task));
    }

    template<typename Tag>
    void basic_thread_pool<Tag>::stop()
    {
        m_executor_work_guard.reset();

        std::call_once(m_join_threads_flag, [this]() { m_thread_group.join_all(); });

        m_io_context.stop();
    }

    template<typename Tag>
    bool basic_thread_pool<Tag>::is_stopped() const
    {
        return m_io_context.stopped();
    }

    template<typename Tag>
    unsigned basic_thread_pool<Tag>::get_num() const noexcept
    {
        return m_thread_num;
    }

    template<typename Tag>
    boost::asio::io_context &basic_thread_pool<Tag>::get_io_context() noexcept
    {
        return m_io_context;
    }

    template<typename Tag>
    const boost::asio::io_context &basic_thread_pool<Tag>::get_io_context() const noexcept
    {
        return m_io_context;
    }

    using thread_pool = basic_thread_pool<thread_pool_default_tag>;
}
