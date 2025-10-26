#include "rpc-server-transport.h"

#include "pseudopipe/pseudopipe.h"

namespace vsh::example {
    int rpc_server_transport::send(const common_lib::buffer &buff)
    {
        return send_impl(buff);
    }

    int rpc_server_transport::send(common_lib::buffer &&buff)
    {
        return send_impl(std::move(buff));
    }

    int rpc_server_transport::recv(common_lib::buffer &buff)
    {
        return pseudopipe::instance_cs().recv(buff);
    }

    void rpc_server_transport::listen()
    {
        //no need implementation (yet)
    }

    void rpc_server_transport::close()
    {
        //no need implementation (yet)
    }

    template<typename Buffer>
    int rpc_server_transport::send_impl(Buffer &&buff)
    {
        return pseudopipe::instance_sc().send(std::forward<Buffer>(buff));
    }
}
