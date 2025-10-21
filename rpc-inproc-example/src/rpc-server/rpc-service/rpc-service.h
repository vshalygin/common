#pragma once

#pragma warning(push, 0)
#include "proto/service.pb.h"
#pragma warning(pop)

#include <memory>

namespace vsh::example {
    class rpc_service
        : public proto::Service
    {
    public:
        rpc_service() = default;

        rpc_service(rpc_service &) = delete;
        rpc_service &operator=(rpc_service &) = delete;

        void GetUser(::google::protobuf::RpcController *controller,
                     const ::vsh::example::proto::GetUserRequest *request,
                     ::vsh::example::proto::GetUserResponse *response,
                     ::google::protobuf::Closure *done) override;
    };
}
