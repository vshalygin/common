#pragma once
#include <boost/asio/io_context.hpp>

#include <functional>

namespace vsh::cl {
    class ithread_pool
    {
    public:
        virtual ~ithread_pool() = default;

        virtual void post(std::function<void()> &&func) const = 0;
        virtual void post(const std::function<void()> &func) const = 0;

        virtual void stop() = 0;
        virtual bool is_stopped() const = 0;

        virtual unsigned get_num() const = 0;

        virtual boost::asio::io_context *get_io_context() = 0;
        virtual const boost::asio::io_context *get_io_context() const = 0;
    };
}
