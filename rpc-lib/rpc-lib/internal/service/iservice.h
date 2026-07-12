#pragma once
#include <rpc-lib/types/future.h>

#include <common-lib/utils/buffer.h>

namespace vshalygin::rpc {
    class iservice
    {
    public:
        virtual ~iservice() = default;

        virtual future<cl::buffer> process_request_async(cl::buffer &&request_message) = 0;
    };
}
