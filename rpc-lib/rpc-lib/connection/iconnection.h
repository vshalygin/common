#pragma once
#include "rpc-lib/types/request-result.h"
#include <common-lib/utils/buffer/buffer.h>
#include <functional>

namespace vsh::rpc {
    class iconnection
    {
    public:
        virtual ~iconnection() = default;

        virtual void request_async(cl::buffer &&message,
                                   std::function<void(request_result, cl::buffer &&)> &&handler) = 0;

        using response_handler_t = std::function<void(cl::buffer &&)>;
        virtual void set_request_handler
            (std::function<void(cl::buffer &&, response_handler_t &&)> &&handler) = 0;

        virtual void set_disconnect_handler(std::function<void()> &&handler) = 0;

        virtual bool is_connected() const = 0;
        virtual void disconnect() = 0;

        virtual size_t get_active_requests_count() const = 0;
    };
}
