#pragma once
#include <rpc-lib/server/server-transport/iserver-connection.h>
#include <rpc-lib/server/server-transport/iserver-transport.h>

namespace vsh::example {
    class rpc_server_transport
        : public rpc::iserver_transport
        , public rpc::iserver_connection
    {
    public:
        rpc_server_transport() = default;

        rpc_server_transport(rpc_server_transport &) = delete;
        rpc_server_transport &operator=(rpc_server_transport &) = delete;

        int send(common_lib::buffer &&buff) override;
        int recv(common_lib::buffer &buff) override;

        void listen() override;
        void close() override;
    };
}
