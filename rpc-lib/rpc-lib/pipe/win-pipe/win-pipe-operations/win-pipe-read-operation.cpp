#ifdef _WIN32
#include "win-pipe-read-operation.h"

namespace vshalygin::rpc::internal {
    win_pipe_read_operation::win_pipe_read_operation(win::pipe_handle::handle_type pipe,
                                                     cl::thread_pool *thread_pool)
        : m_pipe(pipe)
        , m_promise(thread_pool, [](bool b, DWORD ec) { return ftuple(b, ec); })
    {
        m_buffer.push_back(cl::buffer(8192));
    }

    void win_pipe_read_operation::read(std::error_code ec) noexcept
    {
        ec.clear();

        auto res = ::ReadFile(m_pipe,
                              m_buffer.back().data(),
                              static_cast<DWORD>(m_buffer.back().size()),
                              nullptr,
                              reinterpret_cast<OVERLAPPED *>(this));
        if(res == FALSE && ::GetLastError() != ERROR_IO_PENDING) {
            ec.assign(::GetLastError(), std::system_category());
        }
    }

    void win_pipe_read_operation::add_read_bytes(DWORD bytes) noexcept
    {
        m_read_bytes += bytes;
    }

    void win_pipe_read_operation::cancel() noexcept
    {
        ::CancelIo(m_pipe);
    }

    void win_pipe_read_operation::add_buffer_chunk()
    {
        auto prev_size = m_buffer.back().size();
        m_buffer.push_back(cl::buffer(prev_size * 2));
    }

    void win_pipe_read_operation::resolve(bool success, DWORD ec)
    {
        m_promise.resolve(success, ec);
    }

    future<ftuple<bool, DWORD>> win_pipe_read_operation::get_future()
    {
        return m_promise.get_future();

    }

    win_pipe_read_res win_pipe_read_operation::extract_res() noexcept
    {
        return { std::move(m_buffer), m_read_bytes };
    }
}
#endif
