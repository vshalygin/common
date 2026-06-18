#pragma once
#include "pipe-op-res.h"
#include "pipe-wait-res.h"

#include <common-lib/utils/buffer/buffer.h>

#include <optional>
#include <chrono>
#include <functional>

namespace vshalygin::rpc {
    //TODO invalidate должна завершать все pending operations, также все чтения и записи долны завершаться
    //немедленно
    // is_connected и invalidate должны быть синхронизированны
    // при уничтожении вызываем все pending callbacks
    // pipe connection may disrupt spanteneously
    // subscribe to disconnect -> вызываем сразу асинхронно, если disconnect

    class ipipe_endpoint
    {
    public:
        using read_callback_t = std::function<void(pipe_op_res, cl::buffer &&)>;
        using write_callback_t = std::function<void(pipe_op_res)>;

        virtual ~ipipe_endpoint() = default;

        virtual bool is_connected() const = 0;
        virtual void subscribe_to_disconnect(std::function<void()> &&callback) = 0;

        virtual pipe_wait_res wait_connect_for(const std::chrono::microseconds &mcs) const = 0;
        virtual pipe_wait_res wait_connect() const = 0;

        virtual void write_async(cl::buffer &&msg, write_callback_t &&handler) = 0;
        virtual void read_async(read_callback_t &&handler) = 0;

        virtual bool try_to_write_for(cl::buffer &&msg, const std::chrono::microseconds &timeout) = 0;
        virtual std::optional<cl::buffer> try_to_read_for(const std::chrono::microseconds &timeout) = 0;

        virtual void invalidate() = 0;
    };
}
