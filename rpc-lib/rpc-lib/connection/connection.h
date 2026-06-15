#pragma once
#include "iconnection.h"
#include "rpc-lib/transport/itransport.h"
#include "rpc-lib/service/iservice.h"
#include "rpc-lib/types/constants.h"

#include <common-lib/timer/multiple-timer/multiple-timer.h>
#include <common-lib/thread-pool/thread-pool.h>

namespace vshalygin::rpc {
    class connection final
        : public iconnection
    {
    public:
        using response_handler_t = std::function<void(cl::buffer &&)>;
        using request_handler_t = std::function<void(cl::buffer &&, response_handler_t &&)>;

        explicit connection(std::shared_ptr<cl::thread_pool> thread_pool,
                            std::shared_ptr<iservice> service,
                            const std::chrono::microseconds &timeout = RequestTimeout);

        connection(connection &) = delete;
        connection &operator=(connection &) = delete;

        ~connection();

        void start_and_set_transport(std::unique_ptr<itransport> transport) override;

        void request_async(cl::buffer &&message,
                           std::function<void(request_result, cl::buffer &&)> &&handler) override;

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
