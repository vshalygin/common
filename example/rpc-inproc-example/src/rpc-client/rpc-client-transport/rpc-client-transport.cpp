#include "rpc-client-transport.h"

#include "pseudopipe/pseudopipe.h"

namespace vsh::example {
    bool rpc_client_transport::is_connected() const
    {
        return is_connected_.load(std::memory_order_acquire);
    }

    int rpc_client_transport::connect()
    {
        is_connected_.store(true, std::memory_order_release);
        return 0;
    }

    int rpc_client_transport::close()
    {
        is_connected_.store(false, std::memory_order_release);
        return 0;
    }

    int rpc_client_transport::send(cl::buffer &&buff)
    {
        return pseudopipe::instance_cs().send(std::move(buff));
    }

    int rpc_client_transport::recv(cl::buffer &buff)
    {
        return pseudopipe::instance_sc().recv(buff);
    }

    bool rpc_client_transport::is_active() const
    {
        return is_connected();
    }
}