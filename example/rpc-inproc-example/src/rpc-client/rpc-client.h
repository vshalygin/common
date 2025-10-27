#pragma once
#include <rpc-lib/client/client-base.h>

#pragma warning(push, 0)
#include "proto/service.pb.h"
#pragma warning(pop)

#include <memory>

namespace vsh::common_lib {
    class ithread_pool;
}

namespace vsh::example {
    class rpc_client_transport;

    class rpc_client final
        : public rpc::client_base
    {
        using RpcChannel = google::protobuf::RpcChannel;

        rpc_client(std::shared_ptr<common_lib::ithread_pool> thread_pool,
                   std::shared_ptr<rpc_client_transport> rpc_client_transport);

    public:
        explicit rpc_client(std::shared_ptr<common_lib::ithread_pool> thread_pool);

        rpc_client(rpc_client &) = delete;
        rpc_client &operator=(rpc_client &) = delete;

        std::unique_ptr<proto::GetUserResponse> get_user(const proto::GetUserRequest &req);
        std::unique_ptr<proto::GetUserResponse> get_user2(const proto::GetUserRequest &req);

    private:
        proto::Service_Stub service_stub_;
    };
}
