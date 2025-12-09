#pragma once
#include "strand.h"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/thread/thread.hpp>

namespace vsh::cl {
    class thread_pool final
    {
    public:
        explicit thread_pool(unsigned thread_num);
        ~thread_pool();

        thread_pool(thread_pool &) = delete;
        thread_pool &operator=(thread_pool &) = delete;

        template<typename Task>
        void post(Task &&task) const
        {
            boost::asio::post(std::forward<Task>(task));
        }

        void stop();
        bool is_stopped() const;

        unsigned get_num() const;

        boost::asio::io_context *get_io_context();
        const boost::asio::io_context *get_io_context() const;

        std::unique_ptr<strand> create_strand();

    private:
        using io_context = boost::asio::io_context;
        using executor_work_guard = boost::asio::executor_work_guard<io_context::executor_type>;

        const unsigned m_thread_num;
        io_context m_io_context;
        executor_work_guard m_executor_work_guard;

        std::once_flag join_threads_flag_;
        boost::thread_group thread_group_;
    };
}
