#pragma once
#ifdef _WIN32
#include <common-lib/thread/thread.h>

#include <thread>
#include <string>
#include <utility>
#include <mutex>
#include <condition_variable>

#include "win-pipe-operation.h"
#include <win-lib/types/handle.h>

namespace vshalygin::rpc::internal {
    class win_pipe_open_operation
    {
    public:
        using future = cl::future<cl::thread_pool, cl::ftuple<win_pipe_operation_res, win::pipe_handle>>;
        using promise =
            cl::promise<cl::thread_pool,
                        cl::ftuple<win_pipe_operation_res, win::pipe_handle>>;

        explicit win_pipe_open_operation(const std::wstring &pipe_name,
                                         cl::thread_pool *thread_pool);

        win_pipe_open_operation(const win_pipe_open_operation &) = delete;
        win_pipe_open_operation &operator=(const win_pipe_open_operation &) = delete;

        ~win_pipe_open_operation();

        void start();
        void cancel(bool by_timeout) noexcept;

        future get_future();

    private:
        void do_openning();

    private:
        const std::wstring m_full_pipe_name;
        promise m_promise;

        enum class cancel_event
        {
            none,
            canceled,
            canceled_by_timeout
        } m_cancel_event = cancel_event::none;
        std::mutex m_cancel_mtx;
        std::condition_variable m_cancel_cv;

        std::thread m_thread;
    };
}

#endif
