#pragma once
#include "rpc-lib/types/request-result.h"
#include "rpc-lib/types/connection-state.h"
#include <common-lib/utils/buffer/buffer.h>
#include <functional>

namespace vsh::rpc {
    class itransport;

    class iconnection
    {
    public:
        virtual ~iconnection() = default;

        virtual void set_and_start_transport(std::unique_ptr<itransport> transport) = 0;

        virtual void request_async(cl::buffer &&message,
                                   std::function<void(request_result, cl::buffer &&)> &&handler) = 0;

        using response_handler_t = std::function<void(cl::buffer &&)>;
        virtual void set_request_handler
            (std::function<void(cl::buffer &&, response_handler_t &&)> &&handler) = 0;

        virtual void set_change_state_handler(std::function<void(connection_state)> &&handler) = 0;

        virtual bool is_connected() const = 0;
        virtual void disconnect() = 0;

        virtual size_t get_active_requests_count() const = 0;
    };
}
