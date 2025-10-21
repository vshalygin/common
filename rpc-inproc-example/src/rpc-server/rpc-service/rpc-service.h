#pragma once
#include "irpc-service.h"

#pragma warning(push, 0)
#include "proto/service.pb.h"
#pragma warning(pop)

#include <memory>

namespace vsh::example {
    class irpc_server_transport;

    class rpc_service
        : public proto::Service
        , public irpc_service
    {
    public:
        explicit rpc_service(std::unique_ptr<irpc_server_transport> transport);

        rpc_service(rpc_service &) = delete;
        rpc_service &operator=(rpc_service &) = delete;

        void GetUser(::google::protobuf::RpcController *controller,
                     const ::vsh::example::proto::GetUserRequest *request,
                     ::vsh::example::proto::GetUserResponse *response,
                     ::google::protobuf::Closure *done) override;

        void run() override;

    private:
        std::unique_ptr<irpc_server_transport> transport_;
    };
}
