#pragma once
#include "../iclient-pipe-env.h"

#include <common-lib/thread/thread-pool/thread-pool.h>

#include <memory>
#include <string>

namespace vshalygin::rpc {
    class tcp_pipe_client_env
        : public iclient_pipe_env
    {
    public:
        tcp_pipe_client_env(cl::thread_pool *thread_pool,
                            const std::string &ip4_address,
                            uint32_t port);

        tcp_pipe_client_env(const tcp_pipe_client_env &) = delete;
        tcp_pipe_client_env &operator=(const tcp_pipe_client_env &) = delete;

        ~tcp_pipe_client_env();

        pipe_endpoint_future open_pipe(uint64_t client_id) override;
        pipe_endpoint_future open_pipe(uint64_t client_id, std::chrono::milliseconds timeout) override;

        void cancel_pending_client_endpoints(uint64_t client_id) override;
        void cancel_all_pending_client_endpoints() override;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}
