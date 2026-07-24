#pragma once
#ifdef _WIN32
#include <rpc-lib/types/future.h>

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/synchronization/value-locker.h>

#include "win-pipe-operation.h"
#include <win-lib/types/handle.h>

namespace vshalygin::rpc::internal {
    class win_pipe_create_operation
    {
    public:
        explicit win_pipe_create_operation(cl::thread_pool *thread_pool);

        win_pipe_create_operation(const win_pipe_create_operation &) = delete;
        win_pipe_create_operation &operator=(const win_pipe_create_operation &) = delete;

        void set_pipe(win::pipe_handle &&pipe) noexcept;

        void cancel();

        void resolve(bool success, DWORD ec);

        future<ftuple<win::pipe_handle, DWORD>> get_future();

    private:
        //contract: operation must be first
        win_pipe_operation m_operation{ win_pipe_operation_kind::create };

        cl::value_locker<win::pipe_handle> m_pipe;

        promise<ftuple<win::pipe_handle, DWORD>, win::pipe_handle, DWORD> m_promise;
    };
}

#endif

