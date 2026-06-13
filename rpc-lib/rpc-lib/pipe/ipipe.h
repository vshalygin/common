#pragma once
#include "pipe-op-res.h"

#include <common-lib/utils/buffer/buffer.h>

#include <optional>
#include <chrono>
#include <functional>

namespace vshalygin::rpc {
    class ipipe
    {
    public:
        virtual ~ipipe() = default;

        [[nodiscard]] virtual bool is_connected() const = 0;

        virtual bool wait_connect_for(const std::chrono::microseconds &mcs) const = 0;
        virtual bool wait_connect() const = 0;

        virtual bool write_async(cl::buffer &&msg, std::function<void(pipe_op_res)> &&handler) = 0;
        virtual bool read_async(std::function<void(pipe_op_res, cl::buffer &&)> &&handler) = 0;

        virtual bool try_to_write_for(cl::buffer &&msg, const std::chrono::microseconds &timeout) = 0;
        virtual std::optional<cl::buffer> try_to_read_for(const std::chrono::microseconds &timeout) = 0;

        virtual void invalidate() = 0;
    };
}
