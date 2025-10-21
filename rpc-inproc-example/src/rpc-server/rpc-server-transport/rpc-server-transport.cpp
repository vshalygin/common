#include "rpc-server-transport.h"

#include "pseudopipe/pseudopipe.h"

namespace vsh::example {
    int rpc_server_transport::send(const std::string &buff)
    {
        return send_impl(buff);
    }

    int rpc_server_transport::send(std::string &&buff)
    {
        return send_impl(std::move(buff));
    }

    int rpc_server_transport::recv(std::string &buff)
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

    template<typename String>
    int rpc_server_transport::send_impl(String &&buff)
    {
        return pseudopipe::instance_sc().send(std::forward<String>(buff));
    }
}
