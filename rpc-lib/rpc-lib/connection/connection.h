#pragma once
#include "iconnection.h"
#include "rpc-lib/interface/itransport.h"

#include <common-lib/timer/multiple-timer/imultiple-timer.h>
#include <common-lib/thread-pool/ithread-pool.h>

namespace vsh::rpc {
    class connection
        : public iconnection
    {
    public:
        explicit connection(std::unique_ptr<itransport> transport,
                            std::unique_ptr<cl::imultiple_timer> multiple_timer,
                            std::shared_ptr<cl::ithread_pool> thread_pool);

        connection(connection &) = delete;
        connection &operator=(connection &) = delete;

        ~connection();

        void request_async(cl::buffer &&message,
                           std::function<void(request_result, cl::buffer &&)> &&handler) override;

        void set_request_handler
            (std::function<void(cl::buffer &&, response_handler_t &&)> &&handler) override;

        void set_disconnect_handler(std::function<void()> &&handler) override;
        bool is_connected() const override;
        void disconnect() override;

        size_t get_active_requests_count() const override;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}
