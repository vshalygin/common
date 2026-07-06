#pragma once
#include <common-lib/utils/buffer.h>
#include <common-lib/utils/cbuffer-view.h>
namespace vshalygin::rpc {
    class iauthenticator
    {
    public:
        virtual ~iauthenticator() = default;

        virtual cl::buffer create_request() const = 0;
        virtual cl::buffer create_response(cl::cbuffer_view req) const = 0;
        virtual bool check_response(cl::cbuffer_view res) const = 0;
    };
}
