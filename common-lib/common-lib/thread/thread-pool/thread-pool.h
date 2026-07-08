#pragma once
#include "strand.h"
#include "thread-pool-task.h"
#include "common-lib/mpl/function-traits.h"

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

        template<typename Task,
                 std::enable_if_t<!is_thread_pool_task_v<Task>, int> = 0>
        void post(Task &&task) //TODO make const
        {
            boost::asio::post(m_io_context, std::forward<Task>(task));
        }

        template<typename Signature, typename...Args>
        void post(const thread_pool_task<Signature> &task, Args&&...args)
        {
            boost::asio::post(
                m_io_context,
                [task, t = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                    std::apply(task.create_proxy(), std::move(t));
                });
        }

        template<typename Signature, typename...Args>
        void post(thread_pool_task<Signature> &&task, Args&&...args)
        {
            boost::asio::post(
                m_io_context,
                [task = std::move(task), t = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                std::apply(task.create_proxy(), std::move(t));
            });
        }

        void stop();
        bool is_stopped() const;

        unsigned get_num() const noexcept;

        boost::asio::io_context &get_io_context() noexcept;
        const boost::asio::io_context &get_io_context() const noexcept;

        strand create_strand(); //TODO удалить этот метод

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
