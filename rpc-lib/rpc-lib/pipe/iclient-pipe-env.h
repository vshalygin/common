#pragma once
#include "pipe-wait-res.h"

#include <rpc-lib/types/future.h>

#include <memory>
#include <chrono>

namespace vshalygin::rpc {
    class ipipe_endpoint;

    class iclient_pipe_env
    {
    public:
        using pipe_endpoint_future = future<ftuple<pipe_wait_res, std::shared_ptr<ipipe_endpoint>>>;

        virtual ~iclient_pipe_env() = default;

        virtual pipe_endpoint_future open_pipe(uint64_t client_id) = 0;
        virtual pipe_endpoint_future open_pipe(uint64_t client_id, std::chrono::milliseconds timeout) = 0;

        virtual void cancel_pending_client_endpoints(uint64_t client_id) = 0;
        virtual void cancel_all_pending_client_endpoints() = 0;
    };
}
