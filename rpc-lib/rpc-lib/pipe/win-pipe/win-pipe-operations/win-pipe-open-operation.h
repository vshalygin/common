#pragma once
#ifdef _WIN32
#include <rpc-lib/types/future.h>

#include <common-lib/synchronization/event.h>
#include <common-lib/thread/thread-pool/thread-pool.h>

#include <thread>
#include <string>
#include <utility>

#include "win-pipe-operation.h"
#include <win-lib/types/handle.h>

namespace vshalygin::rpc::internal {
    class win_pipe_open_operation
    {
    public:
        using future = future<ftuple<win_pipe_operation_res, win::pipe_handle>>;
        using promise = promise<ftuple<win_pipe_operation_res, win::pipe_handle>,
                                win_pipe_operation_res, win::pipe_handle &&>;

        explicit win_pipe_open_operation(const std::wstring &pipe_name,
                                         cl::thread_pool *thread_pool);

        win_pipe_open_operation(const win_pipe_open_operation &) = delete;
        win_pipe_open_operation &operator=(const win_pipe_open_operation &) = delete;

        win_pipe_open_operation(win_pipe_open_operation &&) = default;
        win_pipe_open_operation &operator=(win_pipe_open_operation &&) = default;

        ~win_pipe_open_operation();

        void start();
        void cancel() noexcept;

        future get_future();

    private:
        void do_openning();

    private:
        std::wstring m_full_pipe_name;
        promise m_promise;
        cl::event m_canceled_event;

        std::thread m_thread;
    };
}

#endif
