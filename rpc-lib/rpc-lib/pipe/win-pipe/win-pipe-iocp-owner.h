#pragma once
#ifdef _WIN32
#include "win-pipe-operations/win-pipe-create-operation.h"

#include "../pipe-wait-res.h"
#include "../ipipe-endpoint.h"

#include <win-lib/types/iocp.h>

#include <string>
#include <thread>

namespace vshalygin::rpc::internal {
    enum class win_pipe_iocp_key
    {
        interrupt_iocp,
        connect_pipe,
        open_pipe,
        read,
        write
    };

    class win_pipe_iocp_owner final
    {
    public:
        explicit win_pipe_iocp_owner(const std::wstring &pipe_name);

        win_pipe_iocp_owner(const win_pipe_iocp_owner &) = delete;
        win_pipe_iocp_owner &operator=(const win_pipe_iocp_owner &) = delete;

        ~win_pipe_iocp_owner();

        void create_pipe_async(win_pipe_create_operation *overlapped);

        void cancel_create(win_pipe_create_operation *overlapped);

    private:
        void run_worker_loop();
        void interrupt_worker_loop() noexcept;

    private:
        const std::wstring m_full_pipe_name;

        win::iocp m_iocp;

        cl::thread_pool m_iocp_thread{ 1 };
        std::thread m_worker;
    };
}
#endif
