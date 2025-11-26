#pragma once
#include "rpc-lib/common/transfer-entry/transfer-entry.h"
#include <common-lib/utils/buffer/buffer.h>

namespace vsh::rpc {
    class irecv_handler
    {
    public:
        virtual ~irecv_handler() = default;

        virtual int process(const cl::buffer &buffer) = 0;//TODO add error ret code
    };
}
