#pragma once
#include <rpc-lib/transport/transport.h>
#include <rpc-lib/types/request-result.h>
#include <rpc-lib/types/future.h>

#include <common-lib/utils/buffer.h>
#include <common-lib/synchronization/guarded-value/guarded-value.h>
#include <common-lib/timer/multiple-timer/multiple-timer.h>

#include <memory>
#include <chrono>

namespace vshalygin::cl {
    class thread_pool;
}

namespace vshalygin::rpc {
    class iservice;
    class ipipe_endpoint;

    class connection final
    {
    public:
        using req_result_future = future<ftuple<request_result, cl::buffer>>;

        connection(std::shared_ptr<cl::thread_pool> thread_pool,
                   std::shared_ptr<ipipe_endpoint> pipe_endpoint,
                   std::shared_ptr<iservice> service,
                   const std::chrono::milliseconds &req_timeout);

        connection(const connection &) = delete;
        connection &operator=(const connection &) = delete;

        ~connection();

        void deactivate();
        bool is_active() const;

        req_result_future request_async(cl::buffer &&message);

        void set_stop_callback(std::function<void()> &&callback);

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}
