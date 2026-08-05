#ifdef _WIN32
#include "win-pipe-write-operation.h"

namespace vshalygin::rpc::internal {
    win_pipe_write_operation::win_pipe_write_operation(win::pipe_handle::handle_type pipe,
                                                       cl::buffer &&buffer,
                                                       cl::thread_pool *thread_pool)
        : m_pipe(pipe)
        , m_buffer(std::move(buffer))
        , m_promise(thread_pool, [](win_pipe_operation_res r) { return r; })
    {}

    void win_pipe_write_operation::start(std::error_code ec) noexcept
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

    void win_pipe_write_operation::resolve()
    {
        auto res = m_res.load(std::memory_order_acquire);
        assert(res != win_pipe_operation_res::unknown);

        m_promise.resolve(res);
    }

    future<win_pipe_operation_res> win_pipe_write_operation::get_future()
    {
        return m_promise.get_future();
    }

    win_pipe_operation_res win_pipe_write_operation::get_result() const noexcept
    {
        return m_res.load(std::memory_order_acquire);
    }

    void win_pipe_write_operation::set_success() noexcept
    {
        m_res.store(win_pipe_operation_res::success, std::memory_order_release);
    }

    void win_pipe_write_operation::set_canceled_if_possible() noexcept
    {
        auto expected = win_pipe_operation_res::unknown;
        m_res.compare_exchange_strong(expected,
                                      win_pipe_operation_res::canceled,
                                      std::memory_order_release,
                                      std::memory_order_relaxed);
    }

    void win_pipe_write_operation::set_timeout_if_possible() noexcept
    {
        auto expected = win_pipe_operation_res::unknown;
        m_res.compare_exchange_strong(expected,
                                      win_pipe_operation_res::timeout,
                                      std::memory_order_release,
                                      std::memory_order_relaxed);
    }

    void win_pipe_write_operation::set_failed_if_possible() noexcept
    {
        auto expected = win_pipe_operation_res::unknown;
        m_res.compare_exchange_strong(expected,
                                      win_pipe_operation_res::failed,
                                      std::memory_order_release,
                                      std::memory_order_relaxed);
    }
}

#endif
