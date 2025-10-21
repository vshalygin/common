#pragma once
#include "irpc-client-transport.h"
#include "irpc-client-connection.h"

#include <atomic>

namespace vsh::example {
    class rpc_client_transport final
        : public irpc_client_transport
        , public irpc_client_connection
    {
    public:
        rpc_client_transport() = default;

        rpc_client_transport(rpc_client_transport &) = delete;
        rpc_client_transport &operator=(rpc_client_transport &) = delete;

        bool is_connected() const override;

        int connect() override;
        int close() override;

        int send(const std::string &buff) override;
        int send(std::string &&buff) override;

        int recv(std::string &buff) override;

    private:
        template<typename String>
        int send_impl(String &&buff);

        std::atomic_bool is_connected_ = false;
    };
}
