#include "rpc-server-transport.h"

#include "pseudopipe/pseudopipe.h"

namespace vsh::example {
    int rpc_server_transport::send(cl::buffer &&buff)
    {
        return pseudopipe::instance_sc().send(std::move(buff));
    }

    int rpc_server_transport::recv(cl::buffer &buff)
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
