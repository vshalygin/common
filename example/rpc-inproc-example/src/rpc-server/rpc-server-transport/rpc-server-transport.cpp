#include "rpc-server-transport.h"

#include "pseudopipe/pseudopipe.h"

namespace vsh::example {
    int rpc_server_transport::send(common_lib::buffer &&buff)
    {
        return pseudopipe::instance_sc().send(std::move(buff));
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
}
