#pragma once
#include "strand.h"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/thread/thread.hpp>

namespace vshalygin::cl {
    class thread_pool final
    {
    public:
        explicit thread_pool(unsigned thread_num);
        ~thread_pool();

        thread_pool(thread_pool &) = delete;
        thread_pool &operator=(thread_pool &) = delete;

        template<typename Task>
        void post(Task &&task)
        {
            boost::asio::post(m_io_context, std::forward<Task>(task));
        }

        void stop();
        bool is_stopped() const;

        unsigned get_num() const noexcept;

        boost::asio::io_context &get_io_context() noexcept;
        const boost::asio::io_context &get_io_context() const noexcept;

        std::unique_ptr<strand> create_strand();

    private:
        using io_context = boost::asio::io_context;
        using executor_work_guard = boost::asio::executor_work_guard<io_context::executor_type>;

        const unsigned m_thread_num;
        io_context m_io_context;
        executor_work_guard m_executor_work_guard;

        std::once_flag m_join_threads_flag;
        boost::thread_group m_thread_group;
    };
}
