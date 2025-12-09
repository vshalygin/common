#pragma once
#include "iconnection.h"
#include "rpc-lib/interface/itransport.h"
#include "rpc-lib/types/constants.h"

#include <common-lib/timer/multiple-timer/multiple-timer.h>
#include <common-lib/thread-pool/thread-pool.h>

namespace vsh::rpc {
    class connection final
        : public iconnection
    {
    public:
        explicit connection(std::shared_ptr<cl::thread_pool> thread_pool,
                            const std::chrono::microseconds &timeout = RequestTimeout);

        connection(connection &) = delete;
        connection &operator=(connection &) = delete;

        ~connection();

        void set_and_start_transport(std::unique_ptr<itransport> transport) override;

        void request_async(cl::buffer &&message,
                           std::function<void(request_result, cl::buffer &&)> &&handler) override;

        void set_request_handler
            (std::function<void(cl::buffer &&, response_handler_t &&)> &&handler) override;

        void set_change_state_handler(std::function<void(connection_state)> &&handler) override;
        bool is_active() const override;
        void stop_transport() override;

        size_t get_active_requests_count() const override;
        size_t get_active_timers_count() const override;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}
