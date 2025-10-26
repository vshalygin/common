#pragma once
#include "rpc-lib/common/transfer-entry/transfer-entry.h"
#include <common-lib/utils/buffer/buffer.h>

namespace vsh::rpc {
    class irecv_event_processor
    {
    public:
        virtual ~irecv_event_processor() = default;

        virtual int process(const common_lib::buffer &buffer) = 0;//TODO add error ret code
    };
}
