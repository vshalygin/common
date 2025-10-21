#pragma once
#include "irpc-client.h"

#include <memory>

namespace google::protobuf {
    class RpcChannel;
}

namespace vsh::example {
    class irpc_client_connection;

    class rpc_client
        : public irpc_client
    {
        using RpcChannel = google::protobuf::RpcChannel;

    public:
        explicit rpc_client(std::unique_ptr<RpcChannel> channel,
                            std::shared_ptr<irpc_client_connection> connection);

        rpc_client(rpc_client &) = delete;
        rpc_client &operator=(rpc_client &) = delete;

        int connect() override;
        int disconnect() override;

        proto::GetUserResponse get_user(const proto::GetUserRequest &req) override;

    private:
        proto::Service_Stub service_stub_;
        std::shared_ptr<irpc_client_connection> connection_;
    };
}
