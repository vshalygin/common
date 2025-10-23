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

        int send(const std::string &buff) override;
        int send(std::string &&buff) override;

        int recv(std::string &buff) override;

        void listen() override;
        void close() override;

    private:
        template<typename String>
        int send_impl(String &&buff);
    };
}
