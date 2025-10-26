#pragma once
#include <common-lib/utils/buffer/buffer.h>

namespace vsh::rpc {
    class iclient_transport
    {
    public:
        virtual ~iclient_transport() = default;

        virtual int send(const common_lib::buffer &buff) = 0;
        virtual int send(common_lib::buffer &&buff) = 0;

        virtual int recv(common_lib::buffer &buff) = 0;
    };
}
