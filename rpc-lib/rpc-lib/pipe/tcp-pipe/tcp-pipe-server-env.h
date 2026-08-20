#pragma once
#include "../iserver-pipe-env.h"

namespace vshalygin::rpc {
    class tcp_pipe_server_env
        : public iserver_pipe_env
    {
    public:
        tcp_pipe_server_env() = default;

        tcp_pipe_server_env(const tcp_pipe_server_env &) = delete;
        tcp_pipe_server_env &operator=(const tcp_pipe_server_env &) = delete;

        pipe_endpoint_future create_pipe() override;
        pipe_endpoint_future create_pipe(std::chrono::milliseconds timeout) override;

        void cancel_pending_server_endpoints() override;
    };
}
