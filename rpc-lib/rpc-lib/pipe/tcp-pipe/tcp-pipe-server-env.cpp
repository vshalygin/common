#include "tcp-pipe-server-env.h"

namespace vshalygin::rpc {
    using pipe_endpoint_future = tcp_pipe_server_env::pipe_endpoint_future;

    pipe_endpoint_future tcp_pipe_server_env::create_pipe()
    {
        //TODO add implementations
        return {};
    }

    pipe_endpoint_future tcp_pipe_server_env::create_pipe(std::chrono::milliseconds /*timeout*/)
    {
        //TODO add implementations
        return {};
    }

    void tcp_pipe_server_env::cancel_pending_server_endpoints()
    {
        //TODO add implementations
    }
}
