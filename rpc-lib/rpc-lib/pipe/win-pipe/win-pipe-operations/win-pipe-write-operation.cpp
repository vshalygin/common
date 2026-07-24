#ifdef _WIN32
#include "win-pipe-write-operation.h"

namespace vshalygin::rpc::internal {
    win_pipe_write_operation::win_pipe_write_operation(win::pipe_handle::handle_type pipe,
                                                       cl::buffer &&buffer,
                                                       cl::thread_pool *thread_pool)
        : m_pipe(pipe)
        , m_buffer(std::move(buffer))
        , m_promise(thread_pool, [](bool success, DWORD ec) { return ftuple(success, ec); })
    {}

    win::pipe_handle::handle_type win_pipe_write_operation::get_pipe() const noexcept
    {
        return m_pipe;
    }

    void win_pipe_write_operation::write(std::error_code ec) noexcept
    {
        ec.clear();

        auto res = ::WriteFile(m_pipe,
                               static_cast<const void *>(m_buffer.data()),
                               static_cast<DWORD>(m_buffer.size()),
                               nullptr,
                               reinterpret_cast<OVERLAPPED *>(this));
        if(res == FALSE && ::GetLastError() != ERROR_IO_PENDING) {
            ec.assign(::GetLastError(), std::system_category());
        }
    }

    void win_pipe_write_operation::cancel() noexcept
    {
        ::CancelIo(m_pipe);
    }

    void win_pipe_write_operation::resolve(bool success, DWORD ec)
    {
        m_promise.resolve(success, ec);
    }

    future<ftuple<bool, DWORD>> win_pipe_write_operation::get_future()
    {
        return m_promise.get_future();
    }
}

#endif
