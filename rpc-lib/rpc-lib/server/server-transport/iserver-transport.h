#pragma once
#include <common-lib/utils/buffer/buffer.h>

namespace vsh::rpc {
    class iserver_transport
    {
    public:
        virtual ~iserver_transport() = default;

        virtual int send(const common_lib::buffer &buff) = 0;
        virtual int send(common_lib::buffer &&buff) = 0;

        virtual int recv(common_lib::buffer &buff) = 0;
    };
}
