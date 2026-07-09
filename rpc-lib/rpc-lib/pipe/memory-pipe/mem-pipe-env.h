#pragma once
#include "../iclient-pipe-env.h"
#include "../iserver-pipe-env.h"
#include <common-lib/thread/thread-pool/thread-pool.h>

namespace vshalygin::rpc {
    class mem_pipe_endpoint;

    class mem_pipe_env final
        : public iclient_pipe_env
        , public iserver_pipe_env
    {
        using pipe_endpoint_future = iclient_pipe_env::pipe_endpoint_future;
        using pipe_endpoint_promise = promise<ftuple<pipe_wait_res, std::shared_ptr<ipipe_endpoint>>,
                                              pipe_wait_res, std::shared_ptr<ipipe_endpoint>>;

    public:
        explicit mem_pipe_env(std::shared_ptr<cl::thread_pool> thread_pool);

        mem_pipe_env(mem_pipe_env &) = delete;
        mem_pipe_env &operator=(mem_pipe_env &) = delete;

        ~mem_pipe_env();

        pipe_endpoint_future create_pipe() override;
        pipe_endpoint_future create_pipe(std::chrono::milliseconds timeout) override;

        pipe_endpoint_future open_pipe() override;
        pipe_endpoint_future open_pipe(std::chrono::milliseconds timeout) override;

        void cancel_pending_client_endpoints() override;
        void cancel_pending_server_endpoints() override;

        size_t get_pending_client_endpoints_count() const;
        size_t get_pending_server_endpoints_count() const;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}
