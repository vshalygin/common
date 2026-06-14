#include "simple-authenticator.h"
#include "../proto/auth.pb.h"

namespace vshalygin::rpc {
    proto::auth_request simple_authenticator::create_request() const
    {
        proto::auth_request req;
        req.set_auth_data("");
        return req;
    }

    bool simple_authenticator::check_request(const proto::auth_request & /*req*/) const
    {
        return true;
    }
}
