#pragma once
#include "../ipipe-endpoint.h"

#include <common-lib/thread/thread.h>

#include <boost/asio/ip/tcp.hpp>

#include <memory>

namespace vshalygin::rpc {
    class tcp_pipe_endpoint
        : public ipipe_endpoint
    {
    public:
        using socket = boost::asio::ip::tcp::socket;

        tcp_pipe_endpoint(cl::thread_pool *thread_pool,
                          socket &&socket);

        tcp_pipe_endpoint(const tcp_pipe_endpoint &) = delete;
        tcp_pipe_endpoint &operator=(const tcp_pipe_endpoint &) = delete;

        ~tcp_pipe_endpoint();

        bool is_connected() const override;
        void set_disconnect_callback(cl::thread_pool_task<void()> &&callback) override;

        write_future write_async(cl::buffer &&msg) override;
        read_future read_async() override;
        write_future write_async(cl::buffer &&msg, std::chrono::milliseconds timeout) override;
        read_future read_async(std::chrono::milliseconds timeout) override;

        void invalidate() override;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}
