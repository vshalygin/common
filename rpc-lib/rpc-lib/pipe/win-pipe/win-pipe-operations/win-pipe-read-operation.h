#pragma once
#ifdef _WIN32
#include <rpc-lib/types/future.h>

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/utils/buffer.h>

#include <vector>
#include <atomic>

#include "win-pipe-operation.h"
#include <win-lib/types/handle.h>

namespace vshalygin::rpc::internal {
    class win_pipe_read_operation
    {
    public:
        explicit win_pipe_read_operation(win::pipe_handle::handle_type pipe,
                                         cl::thread_pool *thread_pool);

        win_pipe_read_operation(const win_pipe_read_operation &) = delete;
        win_pipe_read_operation &operator=(const win_pipe_read_operation &) = delete;

        void start(std::error_code ec) noexcept;
        void add_read_bytes(DWORD bytes) noexcept;

        void cancel() noexcept;

        void add_buffer_chunk();

        void resolve();

        future<ftuple<win_pipe_operation_res, cl::buffer>> get_future();

        win_pipe_operation_res get_result() const noexcept;

        void set_success() noexcept;
        void set_canceled_if_possible() noexcept;
        void set_timeout_if_possible() noexcept;
        void set_failed_if_possible() noexcept;

    private:
        //contract: overlapped must be first
        win_pipe_overlapped m_overlapped{ win_pipe_operation_kind::read };

        win::pipe_handle::handle_type m_pipe;
        std::vector<cl::buffer> m_buffers;

        promise<ftuple<win_pipe_operation_res, cl::buffer>, win_pipe_operation_res, cl::buffer> m_promise;

        DWORD m_read_bytes = 0;

        std::atomic<win_pipe_operation_res> m_res{ win_pipe_operation_res::unknown };
    };
}

#endif
