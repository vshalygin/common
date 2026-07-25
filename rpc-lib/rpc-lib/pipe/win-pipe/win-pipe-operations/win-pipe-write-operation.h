#pragma once
#ifdef _WIN32
#include <rpc-lib/types/future.h>
#include <common-lib/utils/buffer.h>

#include "win-pipe-operation.h"
#include <win-lib/types/handle.h>

namespace vshalygin::rpc::internal {
    class win_pipe_write_operation
    {
    public:
        explicit win_pipe_write_operation(win::pipe_handle::handle_type pipe,
                                          cl::buffer &&buffer,
                                          cl::thread_pool *thread_pool);

        win_pipe_write_operation(const win_pipe_write_operation &) = delete;
        win_pipe_write_operation &operator=(const win_pipe_write_operation &) = delete;

        void write(std::error_code ec) noexcept;
        void cancel() noexcept;

        void resolve(bool success, DWORD ec);

        future<ftuple<bool, DWORD>> get_future();

    private:
        //contract: operation must be first
        win_pipe_operation m_operation{ win_pipe_operation_kind::write }; 

        win::pipe_handle::handle_type m_pipe;
        cl::buffer m_buffer;

        promise<ftuple<bool, DWORD>, bool, DWORD> m_promise;
    };
}

#endif
