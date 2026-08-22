#pragma once
#include "../iserver-pipe-env.h"

#include <common-lib/thread/thread-pool/thread-pool.h>

#include <memory>
#include <string>
#include <cstdint>

namespace vshalygin::rpc {
    class tcp_pipe_server_env
        : public iserver_pipe_env
    {
    public:
        tcp_pipe_server_env(std::shared_ptr<cl::thread_pool> thread_pool,
                            const std::string &ip4_address,
                            uint32_t port);

        tcp_pipe_server_env(const tcp_pipe_server_env &) = delete;
        tcp_pipe_server_env &operator=(const tcp_pipe_server_env &) = delete;

        ~tcp_pipe_server_env();

        pipe_endpoint_future create_pipe() override;
        pipe_endpoint_future create_pipe(std::chrono::milliseconds timeout) override;

        void cancel_pending_server_endpoints() override;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}
