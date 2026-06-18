#pragma once
#include "iconnection.h"
#include "rpc-lib/transport/itransport.h"
#include "rpc-lib/service/iservice.h"
#include "rpc-lib/types/constants.h"

#include <common-lib/timer/multiple-timer/multiple-timer.h>
#include <common-lib/thread-pool/thread-pool.h>

namespace vshalygin::rpc {
    class iconnector;

    class connection final
        : public iconnection
    {
    public:
        using response_handler_t = std::function<void(cl::buffer &&)>;
        using request_handler_t = std::function<void(cl::buffer &&, response_handler_t &&)>;

        explicit connection(std::shared_ptr<iconnector> connector,
                            std::shared_ptr<cl::thread_pool> thread_pool,
                            std::shared_ptr<iservice> service,
                            change_state_handler_t &&change_state_handler,
                            const std::chrono::microseconds &timeout = RequestTimeout);

        connection(connection &) = delete;
        connection &operator=(connection &) = delete;

        ~connection();

        void activate() override;
        void deactivate() override;

        void request_async(cl::buffer &&message,
                           std::function<void(request_result, cl::buffer &&)> &&handler) override;

        bool is_active() const override;

        size_t get_active_requests_count() const;
        size_t get_active_timers_count() const;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}
