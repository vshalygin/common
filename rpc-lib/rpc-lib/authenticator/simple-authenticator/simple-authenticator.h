#pragma once
#include "../iauthenticator.h"

namespace vshalygin::rpc {
    class simple_authenticator
        : public iauthenticator
    {
    public:
        proto::auth_request create_request() const override;
        bool check_request(const proto::auth_request &req) const override;
    };
}
