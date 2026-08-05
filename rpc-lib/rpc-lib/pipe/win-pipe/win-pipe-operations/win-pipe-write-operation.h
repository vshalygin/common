#pragma once
#ifdef _WIN32
#include <rpc-lib/types/future.h>
#include <common-lib/utils/buffer.h>

#include "win-pipe-operation.h"
#include <win-lib/types/handle.h>

#include <atomic>

namespace vshalygin::rpc::internal {
    class win_pipe_write_operation
    {
    public:
        explicit win_pipe_write_operation(win::pipe_handle::handle_type pipe,
                                          cl::buffer &&buffer,
                                          cl::thread_pool *thread_pool);

        win_pipe_write_operation(const win_pipe_write_operation &) = delete;
        win_pipe_write_operation &operator=(const win_pipe_write_operation &) = delete;

        void start(std::error_code ec) noexcept;
        void cancel() noexcept;

        void resolve();

        future<win_pipe_operation_res> get_future();

        win_pipe_operation_res get_result() const noexcept;

        void set_success() noexcept;
        void set_canceled_if_possible() noexcept;
        void set_timeout_if_possible() noexcept;
        void set_failed_if_possible() noexcept;

    private:
        //contract: overlapped must be first
        win_pipe_overlapped m_overlapped{ win_pipe_operation_kind::write };

        win::pipe_handle::handle_type m_pipe;
        cl::buffer m_buffer;

        promise<win_pipe_operation_res, win_pipe_operation_res> m_promise;

        std::atomic<win_pipe_operation_res> m_res{ win_pipe_operation_res::unknown };
    };
}

#endif
