#pragma once
#include "ithread-pool.h"

#include <boost/asio/executor_work_guard.hpp>
#include <boost/thread/thread.hpp>

namespace vsh::cl {
    class thread_pool final
        : public ithread_pool
    {
    public:
        explicit thread_pool(unsigned thread_num);
        ~thread_pool() override;

        thread_pool(thread_pool &) = delete;
        thread_pool &operator=(thread_pool &) = delete;

        void post(std::function<void()> &&func) const override;
        void post(const std::function<void()> &func) const override;

        void stop() override;
        bool is_stopped() const override;

        unsigned get_num() const override;

        boost::asio::io_context *get_io_context() override;
        const boost::asio::io_context *get_io_context() const override;

        std::unique_ptr<istrand> create_strand() override;

    private:
        template<typename Func>
        void post_impl(Func &&func) const;

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
