#pragma once
#ifdef _WIN32
#include <common-lib/thread/thread.h>
#include <common-lib/utils/buffer.h>
#include <common-lib/synchronization/value-locker.h>

#include "win-pipe-operation.h"
#include <win-lib/types/handle.h>

#include <atomic>

namespace vshalygin::rpc::internal {
    class win_pipe_write_operation
        : private win_pipe_overlapped //contract: overlapped must be first
        , public std::enable_shared_from_this<win_pipe_write_operation>
    {
        explicit win_pipe_write_operation(std::shared_ptr<cl::value_locker<win::pipe_handle>> pipe,
                                          cl::buffer &&buffer,
                                          cl::thread_pool *thread_pool);

    public:
        static std::shared_ptr<win_pipe_write_operation> create(
            std::shared_ptr<cl::value_locker<win::pipe_handle>> pipe,
            cl::buffer &&buffer,
            cl::thread_pool *thread_pool);

        win_pipe_write_operation(const win_pipe_write_operation &) = delete;
        win_pipe_write_operation &operator=(const win_pipe_write_operation &) = delete;

        void start(std::error_code &ec) noexcept;
        void cancel() noexcept;

        void resolve();

        cl::future<cl::thread_pool, win_pipe_operation_res> get_future();

        win_pipe_operation_res get_result() const noexcept;

        void set_success() noexcept;
        void set_canceled_if_possible() noexcept;
        void set_timeout_if_possible() noexcept;
        void set_failed_if_possible() noexcept;

    private:
        std::shared_ptr<cl::value_locker<win::pipe_handle>> m_pipe;
        cl::buffer m_buffer;

        cl::promise<cl::thread_pool, win_pipe_operation_res> m_promise;

        std::atomic<win_pipe_operation_res> m_res{ win_pipe_operation_res::unknown };
    };
}

#endif
