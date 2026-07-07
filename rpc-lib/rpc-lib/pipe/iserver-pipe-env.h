#pragma once
#include "pipe-wait-res.h"
#include <rpc-lib/types/future.h>
#include <memory>

namespace vshalygin::rpc {
    class ipipe_endpoint;

    class iserver_pipe_env
    {
    public:
        using pipe_endpoint_future = future<ftuple<pipe_wait_res, std::shared_ptr<ipipe_endpoint>>>;

        virtual ~iserver_pipe_env() = default;

        virtual pipe_endpoint_future create_pipe() = 0;
        virtual void cancel_pending_server_endpoints() = 0;
    };
}
