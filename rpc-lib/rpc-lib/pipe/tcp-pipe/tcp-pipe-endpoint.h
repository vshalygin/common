#pragma once
#include "../ipipe-endpoint.h"

namespace vshalygin::rpc {
    class tcp_pipe_endpoint
        : public ipipe_endpoint
    {
    public:
        tcp_pipe_endpoint() = default;

        tcp_pipe_endpoint(const tcp_pipe_endpoint &) = delete;
        tcp_pipe_endpoint &operator=(const tcp_pipe_endpoint &) = delete;

        bool is_connected() const override;
        void set_disconnect_callback(cl::thread_pool_task<void()> &&callback) override;

        write_future write_async(cl::buffer &&msg) override;
        read_future read_async() override;
        write_future write_async(cl::buffer &&msg, std::chrono::milliseconds timeout) override;
        read_future read_async(std::chrono::milliseconds timeout) override;

        void invalidate() override;
    };
}
