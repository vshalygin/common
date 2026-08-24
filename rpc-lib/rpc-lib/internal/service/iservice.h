#pragma once
#include <common-lib/thread/thread.h>

#include <common-lib/utils/buffer.h>

namespace vshalygin::rpc::internal {
    class iservice
    {
    public:
        virtual ~iservice() = default;

        virtual cl::future<cl::thread_pool, cl::buffer> process_request_async(cl::buffer &&request_message) = 0;
    };
}
