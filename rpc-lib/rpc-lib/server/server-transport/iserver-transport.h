#pragma once
#include <common-lib/utils/buffer/buffer.h>

namespace vsh::rpc {
    class iserver_transport
    {
    public:
        virtual ~iserver_transport() = default;

        virtual int send(cl::buffer &&buff) = 0;
        virtual int recv(cl::buffer &buff) = 0;
    };
}
