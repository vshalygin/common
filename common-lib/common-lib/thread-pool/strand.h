#pragma once
#include "istrand.h"
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>

namespace vsh::cl {
    class strand final
        : public istrand
    {
    public:
        explicit strand(boost::asio::io_context &io_context);

        strand(strand &) = delete;
        strand &operator=(strand &) = delete;

        void post(std::function<void()> &&task) override;

    private:
        boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    };
}
