#pragma once
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>

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
            boost::asio::post(m_strand, std::forward<Task>(task));
        }

    private:
        boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    };
}
