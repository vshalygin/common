#pragma once
#include <rpc-lib/client/client-transport/iclient-connection.h>
#include <rpc-lib/client/client-transport/iclient-transport.h>

#include <atomic>

namespace vsh::example {
    class rpc_client_transport final
        : public rpc::iclient_transport
        , public rpc::iclient_connection
    {
    public:
        rpc_client_transport() = default;

        rpc_client_transport(rpc_client_transport &) = delete;
        rpc_client_transport &operator=(rpc_client_transport &) = delete;

        bool is_connected() const override;

        int connect() override;
        int close() override;

        int send(const common_lib::buffer &buff) override;
        int send(common_lib::buffer &&buff) override;

        int recv(common_lib::buffer &buff) override;

        bool is_active() const override;

    private:
        template<typename Buffer>
        int send_impl(Buffer &&buff);

        std::atomic_bool is_connected_ = false;
    };
}
