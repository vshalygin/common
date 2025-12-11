#pragma once
#include <common-lib/syncronization/guarded-value/guarded-value.h>

#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>
#include <optional>

namespace vsh::cl {
    class strand final
    {
    public:
        explicit strand(boost::asio::io_context &io_context);

        strand(strand &) = delete;
        strand &operator=(strand &) = delete;

        template<typename Task>
        void post(Task &&task) const
        {
            auto decorated_task = [task = std::forward<Task>(task),
                                   executing_thread_id = m_executing_thread_id] () mutable {
                set_current_thread_id(executing_thread_id);
                task();
                clear_thread_id(executing_thread_id);
            };

            boost::asio::post(m_strand, std::move(decorated_task));
        }

        bool is_in_executing_context() const;

    private:
        using thread_id_t = guarded_value<std::optional<std::thread::id>>;

        static void set_current_thread_id(std::shared_ptr<thread_id_t> thread_id);
        static void clear_thread_id(std::shared_ptr<thread_id_t> thread_id);

    private:
        boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
        std::shared_ptr<thread_id_t> m_executing_thread_id;
    };
}
