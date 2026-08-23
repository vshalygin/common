#pragma once
#ifdef _WIN32
#include <rpc-lib/pipe/iclient-pipe-env.h>

#include <common-lib/thread/thread-pool/thread-pool.h>

namespace vshalygin::rpc {
    class win_pipe_client_env
        : public iclient_pipe_env
    {

    public:
        explicit win_pipe_client_env(std::shared_ptr<cl::thread_pool> thread_pool,
                                     const std::wstring &pipe_name);

        win_pipe_client_env(const win_pipe_client_env &) = delete;
        win_pipe_client_env &operator=(const win_pipe_client_env &) = delete;

        ~win_pipe_client_env();

        pipe_endpoint_future open_pipe(uint64_t client_id) override;
        pipe_endpoint_future open_pipe(uint64_t client_id, std::chrono::milliseconds timeout) override;

        void cancel_pending_client_endpoints(uint64_t client_id) override;
        void cancel_all_pending_client_endpoints() override;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}

#endif
