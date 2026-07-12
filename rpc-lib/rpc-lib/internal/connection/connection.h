#pragma once
#include "iconnection.h"

#include <common-lib/utils/buffer.h>

#include <memory>
#include <chrono>

namespace vshalygin::cl {
    class thread_pool;
}

namespace vshalygin::rpc {
    class iservice;
    class ipipe_endpoint;

    class connection final
        : public iconnection
    {
    public:
        connection(std::shared_ptr<cl::thread_pool> thread_pool,
                   std::shared_ptr<ipipe_endpoint> pipe_endpoint,
                   std::shared_ptr<iservice> service,
                   const std::chrono::milliseconds &send_timeout,
                   const std::chrono::milliseconds &recv_timeout);

        connection(const connection &) = delete;
        connection &operator=(const connection &) = delete;

        ~connection();

        void start() override;

        void deactivate() override;
        bool is_active() const override;

        req_result_future request_async(cl::buffer &&message) override;

        void set_stop_callback(cl::thread_pool_task<void()> &&callback) override;

        size_t get_pending_requests_count() const;
        size_t get_active_timers_count() const;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}
