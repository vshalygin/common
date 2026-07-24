#ifdef _WIN32
#include "win-pipe-create-operation.h"

namespace vshalygin::rpc::internal {
    win_pipe_create_operation::win_pipe_create_operation(cl::thread_pool *thread_pool)
        : m_promise(make_promise(thread_pool, [](win::pipe_handle p, DWORD ec) {
                                                  return ftuple(std::move(p), ec);
                                              }))
    {}

    void win_pipe_create_operation::set_pipe(win::pipe_handle &&pipe) noexcept
    {
        assert(!*m_pipe.lock());
        *m_pipe.lock() = std::move(pipe);
    }

    void win_pipe_create_operation::cancel()
    {
        auto locked_pipe = m_pipe.lock();
        assert(*locked_pipe);
        ::CancelIo(locked_pipe->get());
    }

    void win_pipe_create_operation::resolve(bool success, DWORD ec)
    {
        if(success) {
            m_promise.resolve(std::move(*m_pipe.lock()), static_cast<DWORD>(ERROR_SUCCESS));
        } else {
            m_pipe.lock()->reset();
            m_promise.resolve(win::pipe_handle{}, ec);
        }
    }
}

#endif


