#include "tcp-pipe-client-env.h"

namespace vshalygin::rpc {
    using pipe_endpoint_future = tcp_pipe_client_env::pipe_endpoint_future;

    pipe_endpoint_future tcp_pipe_client_env::open_pipe()
    {
        //TODO add implementaion
        return {};
    }

    pipe_endpoint_future tcp_pipe_client_env::open_pipe(std::chrono::milliseconds /*timeout*/)
    {
        //TODO add implementaion
        return {};
    }

    void tcp_pipe_client_env::cancel_pending_client_endpoints()
    {
        //TODO add implementaion
    }
}
