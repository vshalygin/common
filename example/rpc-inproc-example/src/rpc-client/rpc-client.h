#pragma once
#include "irpc-client.h"

#include <memory>

namespace google::protobuf {
    class RpcChannel;
}

namespace vsh::common_lib {
    class ithread_pool;
}

namespace vsh::rpc {
    class iclient_connection;
}

namespace vsh::example {
    class rpc_client final
        : public irpc_client
    {
        using RpcChannel = google::protobuf::RpcChannel;

    public:
        explicit rpc_client(std::unique_ptr<RpcChannel> channel,
                            std::shared_ptr<rpc::iclient_connection> connection);

        rpc_client(rpc_client &) = delete;
        rpc_client &operator=(rpc_client &) = delete;

        int connect() override;
        int disconnect() override;

        std::shared_ptr<proto::GetUserResponse> get_user(const proto::GetUserRequest &req) override;
        std::shared_ptr<proto::GetUserResponse> get_user2(const proto::GetUserRequest &req) override;

    private:
        template<typename Request, typename Response, auto Method>
        std::shared_ptr<Response> call_method(const Request &req);

    private:
        proto::Service_Stub service_stub_;
        std::shared_ptr<rpc::iclient_connection> connection_;
    };
}
