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

    int rpc_client_transport::send(const common_lib::buffer &buff)
    {
        return send_impl(buff);
    }

    int rpc_client_transport::send(common_lib::buffer &&buff)
    {
        return send_impl(std::move(buff));
    }

    int rpc_client_transport::recv(common_lib::buffer &buff)
    {
        return pseudopipe::instance_sc().recv(buff);
    }

    bool rpc_client_transport::is_active() const
    {
        return true; //TODO make correct
    }

    template<typename Buffer>
    int rpc_client_transport::send_impl(Buffer &&buff)
    {
        return pseudopipe::instance_cs().send(std::forward<Buffer>(buff));
    }
}