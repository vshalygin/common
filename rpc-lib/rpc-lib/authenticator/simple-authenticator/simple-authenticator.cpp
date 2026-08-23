#include "simple-authenticator.h"

namespace vshalygin::rpc {
    cl::buffer simple_authenticator::create_request() const
    {
        return cl::buffer{1};
    }

    cl::buffer simple_authenticator::create_response(cl::cbuffer_view) const
    {
        return cl::buffer{1};
    }

    bool simple_authenticator::check_request(cl::cbuffer_view) const
    {
        return true;
    }

    bool simple_authenticator::check_response(cl::cbuffer_view) const
    {
        return true;
    }
}
