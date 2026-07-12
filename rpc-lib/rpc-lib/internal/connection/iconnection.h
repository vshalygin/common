#pragma once
#include <rpc-lib/types/future.h>
#include <rpc-lib/types/request-result.h>

#include <common-lib/utils/buffer.h>
#include <common-lib/thread/thread-pool/thread-pool-task.h>

namespace vshalygin::rpc {
    class iconnection
    {
    public:
        using req_result_future = future<ftuple<request_result, cl::buffer>>;

        virtual ~iconnection() = default;

        virtual void start() = 0;

        virtual void deactivate() = 0;
        virtual bool is_active() const = 0;

        virtual req_result_future request_async(cl::buffer &&message) = 0;

        virtual void set_stop_callback(cl::thread_pool_task<void()> &&callback) = 0;
    };
}
