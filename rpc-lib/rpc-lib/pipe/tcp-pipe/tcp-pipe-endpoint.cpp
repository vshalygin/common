#include "tcp-pipe-endpoint.h"

namespace vshalygin::rpc {
    using write_future = tcp_pipe_endpoint::write_future;
    using read_future = tcp_pipe_endpoint::read_future;

    bool tcp_pipe_endpoint::is_connected() const
    {
        //TODO add implementaion
        return {};
    }

    void tcp_pipe_endpoint::set_disconnect_callback(cl::thread_pool_task<void()> && /*callback*/)
    {
        //TODO add implementaion
    }

    write_future tcp_pipe_endpoint::write_async(cl::buffer && /*msg*/)
    {
        //TODO add implementaion
        return {};
    }

    read_future tcp_pipe_endpoint::read_async()
    {
        //TODO add implementaion
        return {};
    }

    write_future tcp_pipe_endpoint::write_async(cl::buffer && /*msg*/, std::chrono::milliseconds /*timeout*/)
    {
        //TODO add implementaion
        return {};
    }

    read_future tcp_pipe_endpoint::read_async(std::chrono::milliseconds /*timeout*/)
    {
        //TODO add implementaion
        return {};
    }

    void invalidate()
    {
        //TODO add implementaion
    }
}
