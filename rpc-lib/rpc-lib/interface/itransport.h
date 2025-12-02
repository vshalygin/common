#pragma once
#include <common-lib/utils/buffer/buffer.h>

#include <functional>

namespace vsh::rpc {
    class itransport
    {
    public:
        virtual ~itransport() = default;

        virtual void send_async(cl::buffer &&message,
                                std::function<void()> &&error_handler) const = 0;
        virtual void recv_async(std::function<void(cl::buffer &&)> &&handler) const = 0;

        virtual void stop() = 0;
        virtual bool is_stopped() const = 0;

        virtual void set_stop_handler(std::function<void()> &&handler) = 0;
    };
}
