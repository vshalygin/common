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

        virtual pipe_endpoint_future open_pipe() = 0;
        virtual pipe_endpoint_future open_pipe(std::chrono::milliseconds timeout) = 0;

        virtual void cancel_pending_client_endpoints() = 0;
    };
}
