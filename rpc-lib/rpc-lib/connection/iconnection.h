#pragma once
#include "rpc-lib/types/request-result.h"
#include "rpc-lib/types/connection-state.h"
#include <common-lib/utils/buffer/buffer.h>
#include <functional>
#include <chrono>

namespace vshalygin::cl {
    class multiple_timer;
    class thread_pool;
}

namespace vshalygin::rpc {
    class itransport;

    class iconnection
    {
    public:
        using change_state_handler_t = std::function<void(connection_state)>;

        virtual ~iconnection() = default;

        virtual void start_and_set_transport(std::unique_ptr<itransport> transport) = 0;

        virtual void request_async(cl::buffer &&message,
                                   std::function<void(request_result, cl::buffer &&)> &&handler) = 0;

        virtual bool is_active() const = 0;
        virtual void stop_transport() = 0;

        virtual size_t get_active_requests_count() const = 0;
        virtual size_t get_active_timers_count() const = 0;
    };
}
