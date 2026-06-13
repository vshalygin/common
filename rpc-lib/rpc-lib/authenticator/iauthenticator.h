#pragma once
#pragma warning(push, 0)
#include "proto/auth.pb.h"
#pragma warning(pop)

namespace vshalygin::rpc {
    class iauthenticator
    {
    public:
        virtual ~iauthenticator() = default;

        virtual proto::auth_request create_request() const = 0;
        virtual bool check_request(const proto::auth_request &req) const = 0;
    };
}
