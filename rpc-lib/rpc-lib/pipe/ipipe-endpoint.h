#pragma once
#include "pipe-op-res.h"
#include "pipe-wait-res.h"
#include <rpc-lib/types/future.h>

#include <common-lib/utils/buffer.h>
#include <common-lib/thread/thread-pool/thread-pool-task.h>

#include <optional>
#include <chrono>

namespace vshalygin::rpc {
    //TODO invalidate должна завершать все pending operations, также все чтения и записи долны завершаться
    //немедленно ???
    // is_connected и invalidate должны быть синхронизированны
    // при уничтожении вызываем все pending callbacks
    // pipe connection may disrupt spanteneously
    // subscribe to disconnect -> вызываем сразу асинхронно, если disconnect
    // set_disconnect_callback вызывается только один раз
    // callback не должен вызываться в потоке set_disconnect_callback
    // set_disconnect_callback колбек вызывается, если на момент установки уже установлен

    class ipipe_endpoint
    {
    public:
        using read_future = future<ftuple<pipe_op_res, cl::buffer>>;
        using write_future = future<pipe_op_res>;

        virtual ~ipipe_endpoint() = default;

        virtual bool is_connected() const = 0;
        virtual void set_disconnect_callback(cl::thread_pool_task<void()> &&callback) = 0;

        virtual write_future write_async(cl::buffer &&msg) = 0;
        virtual read_future read_async() = 0;
        virtual write_future write_async(cl::buffer &&msg, const std::chrono::milliseconds &timeout) = 0;
        virtual read_future read_async(const std::chrono::milliseconds &timeout) = 0;

        virtual void invalidate() = 0;
    };
}
