#pragma once
#include <rpc-lib/client/client-connection/iclient-connection.h>
#include <rpc-lib/common/transport/itransport.h>

#include <atomic>

namespace vsh::example {
    class rpc_client_transport final
        : public rpc::itransport
        , public rpc::iclient_connection
    {
    public:
        rpc_client_transport() = default;

        rpc_client_transport(rpc_client_transport &) = delete;
        rpc_client_transport &operator=(rpc_client_transport &) = delete;

        bool is_connected() const override;

        int connect() override;
        int close() override;

        int send(cl::buffer &&buff) override;

        int recv(cl::buffer &buff) override;

        bool is_active() const override;

    private:
        std::atomic_bool is_connected_ = false;
    };
}
