#pragma once
#include "../iclient-pipe-env.h"

namespace vshalygin::rpc {
    class tcp_pipe_client_env
        : public iclient_pipe_env
    {
    public:
        tcp_pipe_client_env() = default;

        tcp_pipe_client_env(const tcp_pipe_client_env &) = delete;
        tcp_pipe_client_env &operator=(const tcp_pipe_client_env &) = delete;

        pipe_endpoint_future open_pipe() override;
        pipe_endpoint_future open_pipe(std::chrono::milliseconds timeout) override;

        void cancel_pending_client_endpoints() override;
    };
}
