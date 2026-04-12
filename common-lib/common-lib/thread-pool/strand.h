#pragma once
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>
#include <type_traits>
#include <atomic>

namespace vshalygin::cl {
    class strand final
    {
    public:
        explicit strand(boost::asio::io_context &io_context);

        strand(strand &) = delete;
        strand &operator=(strand &) = delete;

        template<typename Task>
        void post(Task &&task) const;

        bool is_in_executing_context() const;

    private:
        using thread_id_t = std::atomic<std::thread::id>;

        class thread_id_guard
        {
        public:
            explicit thread_id_guard(std::shared_ptr<thread_id_t> thread_id);
            ~thread_id_guard();

            thread_id_guard(thread_id_guard &) = delete;
            thread_id_guard &operator=(thread_id_guard &) = delete;

        private:
            std::shared_ptr<thread_id_t> m_thread_id;
        };

    private:
        boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
        std::shared_ptr<thread_id_t> m_executing_thread_id;
    };

    template<typename Task>
    void strand::post(Task &&task) const
    {
        auto decorated_task = [task = std::make_unique<std::decay_t<Task>>(std::forward<Task>(task)),
                               executing_thread_id = m_executing_thread_id]() mutable {
            thread_id_guard guard(executing_thread_id);
            (*task)();
            task.reset();
        };

        boost::asio::post(m_strand, std::move(decorated_task));
    }
}
