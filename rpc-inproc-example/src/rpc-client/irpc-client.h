#pragma once
#pragma warning(push, 0)
#include "proto/service.pb.h"
#pragma warning(pop)

namespace vsh::example {
    class irpc_client
    {
    public:
        virtual ~irpc_client() = default;

        virtual int connect() = 0;
        virtual int disconnect() = 0;

        virtual proto::GetUserResponse get_user(const proto::GetUserRequest &req) = 0;
    };
}
