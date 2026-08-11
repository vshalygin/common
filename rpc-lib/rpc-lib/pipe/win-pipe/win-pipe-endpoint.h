#pragma once
#ifdef _WIN32
#include "../ipipe-endpoint.h"

#include <common-lib/thread/thread-pool/thread-pool.h>

#include "win-pipe-iocp-owner.h"
#include <win-lib/types/handle.h>

#include <memory>

namespace vshalygin::rpc {
    class win_pipe_endpoint
        : public ipipe_endpoint
    {
    public:
        explicit win_pipe_endpoint(win::pipe_handle &&handle,
                                   std::shared_ptr<internal::win_pipe_iocp_owner> iocp_owner,
                                   std::shared_ptr<cl::thread_pool> thread_pool);

        win_pipe_endpoint(const win_pipe_endpoint &) = delete;
        win_pipe_endpoint &operator=(const win_pipe_endpoint &) = delete;

        ~win_pipe_endpoint();

        bool is_connected() const override;
        void set_disconnect_callback(cl::thread_pool_task<void()> &&callback) override;

        write_future write_async(cl::buffer &&msg) override;
        read_future read_async() override;
        write_future write_async(cl::buffer &&msg, std::chrono::milliseconds timeout) override;
        read_future read_async(std::chrono::milliseconds timeout) override;

        void invalidate() override;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}

#endif
