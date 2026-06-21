#pragma once
#include <rpc-lib/pipe/pipe-op-res.h>
#include <common-lib/utils/buffer/buffer.h>
#include <common-lib/thread-pool/thread-pool-task.h>

#include <functional>
#include <atomic>

namespace vshalygin::cl {
    class thread_pool;
}

namespace vshalygin::rpc {
    class ipipe_endpoint;

    class transport final
    {
    public:
        explicit transport(std::shared_ptr<cl::thread_pool> thread_pool,
                           std::shared_ptr<ipipe_endpoint> pipe_endpoint,
                           cl::thread_pool_task<void()> &&start_callback,
                           cl::thread_pool_task<void()> &&stop_callback);

        transport(const transport &) = delete;
        transport &operator=(const transport &) = delete;
        transport(transport &&) = default;
        transport &operator=(transport &&) = default;

        ~transport();

        using send_callback_t = std::function<void(pipe_op_res)>;
        using recv_callback_t = std::function<void(pipe_op_res, cl::buffer &&)>;

        void send_async(cl::buffer &&message, send_callback_t &&callback);
        void recv_async(recv_callback_t &&callback);

        void stop();
        bool is_running() const;

    private:
        std::shared_ptr<ipipe_endpoint> m_pipe_endpoint;

        std::atomic_bool m_stopped_requested = false;
    };
}
