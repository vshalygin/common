#pragma once
#include "pipe-op-res.h"

#include <common-lib/thread/thread.h>

#include <common-lib/utils/buffer.h>
#include <common-lib/thread/thread-pool/thread-pool-task.h>

#include <chrono>

namespace vshalygin::rpc {
    // Requirements for the ipipe_endpoint interface implementation:
    // 1) The 'invalidate' method must complete all pending operations with the pipe_op_res::canceled result code.
    // 2) Object destruction must be equivalent to calling the 'invalidate' method with regard to operation completion.
    // 3) The 'is_connected' and 'invalidate' methods must be synchronized.
    // 4) The connection may be lost unexpectedly. In this case, all pending operations must complete with
    //    pipe_op_res::failed result code and the disconnect_callback must be invoked.
    // 5) If the connection is unavailable, all new operations must be completed immediately
    //    with the pipe_op_res::failed result code.
    // 6) Completion of pending operations due to a timeout must be supported. In this case,
    //    the pipe_op_res::timeout result code must be returned.
    // 7) If the connection has already been lost at the moment the disconnect_callback is set, the disconnect_callback
    //    must be invoked immediately.
    // 8) The object is created with an active connection. Reconnection is not supported.
    // 9) read_async may unexpectedly fail only on disconnect event.
    // 10) read_async must read no more than MaxTransferMessageSize for one message.
    //     Otherwise consider it as pipe_op_res::failed

    class ipipe_endpoint
    {
    public:
        using read_future = cl::future<cl::thread_pool, cl::ftuple<pipe_op_res, cl::buffer>>;
        using write_future = cl::future<cl::thread_pool, pipe_op_res>;

        virtual ~ipipe_endpoint() = default;

        virtual bool is_connected() const = 0;
        virtual void set_disconnect_callback(cl::thread_pool_task<void()> &&callback) = 0;

        virtual write_future write_async(cl::buffer &&msg) = 0;
        virtual read_future read_async() = 0;
        virtual write_future write_async(cl::buffer &&msg, std::chrono::milliseconds timeout) = 0;
        virtual read_future read_async(std::chrono::milliseconds timeout) = 0;

        virtual void invalidate() = 0;
    };
}
