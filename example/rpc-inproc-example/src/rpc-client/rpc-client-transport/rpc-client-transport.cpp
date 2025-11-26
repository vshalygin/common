#include "rpc-client-transport.h"

#include "pseudopipe/pseudopipe.h"

namespace vsh::example {
    bool rpc_client_transport::is_connected() const
    {
        return m_is_connected.load(std::memory_order_acquire);
    }

    int rpc_client_transport::connect()
    {
        m_is_connected.store(true, std::memory_order_release);
        return 0;
    }

    int rpc_client_transport::close()
    {
        m_is_connected.store(false, std::memory_order_release);
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