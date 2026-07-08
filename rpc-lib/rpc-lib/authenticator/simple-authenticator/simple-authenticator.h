#pragma once
#include "../iauthenticator.h"

namespace vshalygin::rpc {
    class simple_authenticator
        : public iauthenticator
    {
    public:
        cl::buffer create_request() const override;
        cl::buffer create_response(cl::cbuffer_view req) const override;
        bool check_request(cl::cbuffer_view req) const override;
        bool check_response(cl::cbuffer_view res) const override;
    };
}
