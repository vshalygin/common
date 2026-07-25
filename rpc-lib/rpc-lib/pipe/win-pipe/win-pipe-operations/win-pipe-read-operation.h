#pragma once
#ifdef _WIN32
#include <rpc-lib/types/future.h>

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/utils/buffer.h>

#include <vector>

#include "win-pipe-operation.h"
#include <win-lib/types/handle.h>

namespace vshalygin::rpc::internal {
    struct win_pipe_read_res
    {
        std::vector<cl::buffer> buffer;
        size_t size = 0;
    };

    class win_pipe_read_operation
    {
    public:
        explicit win_pipe_read_operation(win::pipe_handle::handle_type pipe,
                                         cl::thread_pool *thread_pool);

        win_pipe_read_operation(const win_pipe_read_operation &) = delete;
        win_pipe_read_operation &operator=(const win_pipe_read_operation &) = delete;

        void read(std::error_code ec) noexcept;
        void add_read_bytes(DWORD bytes) noexcept;

        void cancel() noexcept;

        void add_buffer_chunk();

        void resolve(bool success, DWORD ec);

        future<ftuple<bool, DWORD>> get_future();

        win_pipe_read_res extract_res() noexcept;

    private:
        //contract: operation must be first
        win_pipe_operation m_operation{ win_pipe_operation_kind::read };

        win::pipe_handle::handle_type m_pipe;
        std::vector<cl::buffer> m_buffer;

        promise<ftuple<bool, DWORD>, bool, DWORD> m_promise;

        DWORD m_read_bytes = 0;
    };
}

#endif
