#pragma once
#ifdef _WIN32
#include <rpc-lib/types/future.h>

#include <win-lib/types/handle.h>

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/synchronization/value-locker.h>

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

    private:
        OVERLAPPED m_overlapped{}; //contract: must be first
        cl::value_locker<win::pipe_handle> m_pipe;

        promise<ftuple<win::pipe_handle, DWORD>, win::pipe_handle, DWORD> m_promise;
    };
}

#endif

