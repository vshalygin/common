#pragma once
#ifdef _WIN32
#include "win-pipe-operations/win-pipe-create-operation.h"
#include "win-pipe-operations/win-pipe-open-operation.h"
#include "win-pipe-operations/win-pipe-write-operation.h"
#include "win-pipe-operations/win-pipe-read-operation.h"

#include "../pipe-wait-res.h"
#include "../ipipe-endpoint.h"

#include <win-lib/types/iocp.h>

#include <string>
#include <thread>
#include <chrono>

namespace vshalygin::rpc::internal {
    enum class win_pipe_iocp_key
    {
        interrupt_iocp,
        process_operation
    };

    class win_pipe_iocp_owner final
        : public std::enable_shared_from_this<win_pipe_iocp_owner>
    {
        win_pipe_iocp_owner();

    public:
        static std::shared_ptr<win_pipe_iocp_owner> create();

        win_pipe_iocp_owner(const win_pipe_iocp_owner &) = delete;
        win_pipe_iocp_owner &operator=(const win_pipe_iocp_owner &) = delete;

        ~win_pipe_iocp_owner();

        void create_pipe_async(const std::wstring &pipe_name, win_pipe_create_operation *overlapped);
        void cancel_create(win_pipe_create_operation *overlapped);

        future<ftuple<pipe_wait_res, win::pipe_handle>> open_pipe_async(win_pipe_open_operation *op);
        void cancel_open_pipe(win_pipe_open_operation *op);

        void read_async(win_pipe_read_operation *overlapped);
        void cancel_read(win_pipe_read_operation *overlapped);

        void write_async(win_pipe_write_operation *overlapped);
        void cancel_write(win_pipe_write_operation *overlapped);

    private:
        void run_worker_loop();
        void interrupt_worker_loop() noexcept;

    private:
        win::iocp m_iocp;

        cl::thread_pool m_iocp_thread{ 1 };
        std::thread m_worker;
    };
}
#endif
