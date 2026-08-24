#pragma once
#ifdef _WIN32
#include <common-lib/thread/thread.h>
#include <common-lib/utils/buffer.h>
#include <common-lib/synchronization/value-locker.h>

#include <vector>
#include <atomic>

#include "win-pipe-operation.h"
#include <win-lib/types/handle.h>

namespace vshalygin::rpc::internal {
    class win_pipe_read_operation
        : private win_pipe_overlapped //contract: overlapped must be first
        , public std::enable_shared_from_this<win_pipe_read_operation>
    {
        explicit win_pipe_read_operation(std::shared_ptr<cl::value_locker<win::pipe_handle>> pipe,
                                         cl::thread_pool *thread_pool);

    public:
        static std::shared_ptr<win_pipe_read_operation> create(
            std::shared_ptr<cl::value_locker<win::pipe_handle>> pipe,
            cl::thread_pool *thread_pool);

        win_pipe_read_operation(const win_pipe_read_operation &) = delete;
        win_pipe_read_operation &operator=(const win_pipe_read_operation &) = delete;

        void start(std::error_code &ec) noexcept;
        void add_read_bytes(DWORD bytes) noexcept;

        void cancel() noexcept;

        bool add_buffer_chunk();

        void resolve();

        cl::future<cl::thread_pool, cl::ftuple<win_pipe_operation_res, cl::buffer>> get_future();

        win_pipe_operation_res get_result() const noexcept;

        void set_success() noexcept;
        void set_canceled_if_possible() noexcept;
        void set_timeout_if_possible() noexcept;
        void set_failed_if_possible() noexcept;

    private:
        std::shared_ptr<cl::value_locker<win::pipe_handle>> m_pipe;
        std::vector<cl::buffer> m_buffers;

        cl::promise<cl::thread_pool, cl::ftuple<win_pipe_operation_res, cl::buffer>(win_pipe_operation_res, cl::buffer)> m_promise;

        std::atomic<DWORD> m_read_bytes = 0;

        std::atomic<win_pipe_operation_res> m_res{ win_pipe_operation_res::unknown };
    };
}

#endif
