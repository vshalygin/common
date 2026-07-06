#pragma once
#include <rpc-lib/types/future.h>
#include <common-lib/utils/buffer.h>
#include <memory>
#include <functional>

namespace vshalygin::rpc {
    class iservice
    {
    public:
        virtual ~iservice() = default;

        virtual future<cl::buffer> process_request(cl::buffer &&request_message) = 0;
    };
}
