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

        strand(strand &&) = default;
        strand &operator=(strand &&) = default;

        template<typename Task>
        void post(Task &&task) const
        {
            boost::asio::post(m_strand, std::forward<Task>(task));
        }

        template<typename Task>
        void dispatch(Task &&task) const
        {
            boost::asio::dispatch(m_strand, std::forward<Task>(task));
        }

    private:
        boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    };
}
