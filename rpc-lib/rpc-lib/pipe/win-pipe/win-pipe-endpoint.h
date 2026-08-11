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
        , public std::enable_shared_from_this<win_pipe_endpoint>
    {
        explicit win_pipe_endpoint(win::pipe_handle &&handle,
                                   std::shared_ptr<internal::win_pipe_iocp_owner> iocp_owner,
                                   std::shared_ptr<cl::thread_pool> thread_pool);
    public:
        static std::shared_ptr<ipipe_endpoint> create(win::pipe_handle &&handle,
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
        void complete_write_op();
        void complete_read_op();

        void invoke_disconnect_callbacks();

        void invalidate_impl(bool by_cancel);

    private:
        using write_op = internal::win_pipe_write_operation;
        using read_op = internal::win_pipe_read_operation;

        mutable std::mutex m_pipe_mtx;
        std::vector<cl::thread_pool_task<void()>> m_disconnect_callbacks;
        win::pipe_handle m_pipe;

        std::shared_ptr<internal::win_pipe_iocp_owner> m_iocp_owner;
        std::shared_ptr<cl::thread_pool> m_thread_pool;

        mutable std::mutex m_write_op_mtx;
        std::list<write_op> m_write_ops;

        mutable std::mutex m_read_op_mtx;
        std::list<read_op> m_read_ops;
    };
}

#endif
