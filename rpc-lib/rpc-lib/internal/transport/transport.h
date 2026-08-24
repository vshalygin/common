#pragma once
#include <rpc-lib/pipe/pipe-op-res.h>

#include <common-lib/utils/buffer.h>
#include <common-lib/thread/thread.h>

namespace vshalygin::rpc {
    class ipipe_endpoint;
}

namespace vshalygin::rpc::internal {
    class transport final
    {
    public:
        explicit transport(std::shared_ptr<ipipe_endpoint> pipe_endpoint,
                           const std::chrono::milliseconds &send_timeout);

        transport(const transport &) = delete;
        transport &operator=(const transport &) = delete;

        ~transport();

        using send_future = cl::future<cl::thread_pool, pipe_op_res>;
        using recv_future = cl::future<cl::thread_pool, cl::ftuple<pipe_op_res, cl::buffer>>;

        send_future send_async(cl::buffer &&message);
        recv_future recv_async();

        void stop();
        bool is_running() const;

        void set_stop_callback(cl::thread_pool_task<void()> &&stop_callback);

    private:
        const std::chrono::milliseconds m_send_timeout;
        std::shared_ptr<ipipe_endpoint> m_pipe_endpoint;
    };
}
