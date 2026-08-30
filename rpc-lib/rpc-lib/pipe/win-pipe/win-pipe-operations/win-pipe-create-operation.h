#pragma once
#ifdef _WIN32
#include <common-lib/thread/thread.h>
#include <common-lib/synchronization/value-locker.h>

#include <string>
#include <atomic>
#include <functional>

#include "win-pipe-operation.h"
#include <win-lib/types/handle.h>

namespace vshalygin::rpc::internal {
    class win_pipe_create_operation
        : private win_pipe_overlapped //contract: overlapped must be first
        , public std::enable_shared_from_this<win_pipe_create_operation>
    {
        explicit win_pipe_create_operation(const std::wstring &pipe_name,
                                           cl::thread_pool *thread_pool);

    public:
        using future = cl::future<cl::thread_pool, cl::ftuple<win_pipe_operation_res, win::pipe_handle>>;
        using promise =
            cl::promise<cl::thread_pool,
                        cl::ftuple<win_pipe_operation_res, win::pipe_handle>>;

        static std::shared_ptr<win_pipe_create_operation> create(const std::wstring &pipe_name,
                                                                 cl::thread_pool *thread_pool);

        win_pipe_create_operation(const win_pipe_create_operation &) = delete;
        win_pipe_create_operation &operator=(const win_pipe_create_operation &) = delete;

        bool create_pipe();
        bool start_wait_connect();
        void cancel(bool by_timeout) noexcept;

        void exec(const std::function<void(win::pipe_handle::handle_type)> &f);

        void resolve();
        future get_future();

        void set_success() noexcept;
        void set_canceled_if_possible() noexcept;
        void set_timeout_if_possible() noexcept;
        void set_failed_if_possible() noexcept;

    private:
        const std::wstring m_full_pipe_name;

        cl::value_locker<win::pipe_handle> m_pipe;

        promise m_promise;

        std::atomic<win_pipe_operation_res> m_res{ win_pipe_operation_res::unknown };
    };
}

#endif

