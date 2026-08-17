#pragma once
#ifdef _WIN32
#include "../iserver-pipe-env.h"

#include <common-lib/thread/thread-pool/thread-pool.h>

#include <string>
#include <memory>

namespace vshalygin::rpc {
    class win_pipe_server_env
        : public iserver_pipe_env
    {
    public:
        explicit win_pipe_server_env(const std::wstring &pipe_name,
                                     std::shared_ptr<cl::thread_pool> thread_pool);

        win_pipe_server_env(const win_pipe_server_env &) = delete;
        win_pipe_server_env &operator=(const win_pipe_server_env &) = delete;

        ~win_pipe_server_env();

        pipe_endpoint_future create_pipe() override;
        pipe_endpoint_future create_pipe(std::chrono::milliseconds timeout) override;
        void cancel_pending_server_endpoints() override;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}

#endif
