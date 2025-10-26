#pragma once
#include <common-lib/utils/buffer/buffer.h>

namespace vsh::rpc {
    class itransport
    {
    public:
        virtual ~itransport() = default;

        virtual int send(common_lib::buffer &&buff) = 0;
        virtual int recv(common_lib::buffer &buff) = 0;

        virtual bool is_active() const = 0;
    };
}
