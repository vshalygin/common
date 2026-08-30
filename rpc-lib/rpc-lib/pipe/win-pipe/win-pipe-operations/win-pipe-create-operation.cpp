#ifdef _WIN32
#include "win-pipe-create-operation.h"

namespace vshalygin::rpc::internal {
    std::shared_ptr<win_pipe_create_operation> win_pipe_create_operation::create(const std::wstring &pipe_name,
                                                                                 cl::thread_pool *thread_pool)
    {
        return std::shared_ptr<win_pipe_create_operation>(new win_pipe_create_operation(pipe_name, thread_pool));
    }

    win_pipe_create_operation::win_pipe_create_operation(const std::wstring &pipe_name,
                                                         cl::thread_pool *thread_pool)
        : win_pipe_overlapped(win_pipe_operation_kind::create)
        , m_full_pipe_name(L"\\\\.\\pipe\\" + pipe_name)
        , m_promise(thread_pool)
    {}

    bool win_pipe_create_operation::create_pipe()
    {
        win::pipe_handle pipe(
            ::CreateNamedPipeW(m_full_pipe_name.c_str(),
                               PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                               PIPE_UNLIMITED_INSTANCES,
                               8192,
                               8192,
                               0,
                               NULL));
        if(pipe.empty()) {
            set_failed_if_possible();
            return false;
        }

        *m_pipe.lock() = std::move(pipe);
        return true;
    }

    bool win_pipe_create_operation::start_wait_connect()
    {
        auto r = ::ConnectNamedPipe(m_pipe.lock()->get(), reinterpret_cast<OVERLAPPED *>(this));
        if(r || ::GetLastError() == ERROR_IO_PENDING) {
            return true;
        } else {
            if(::GetLastError() == ERROR_PIPE_CONNECTED) {
                set_success();
            } else {
                set_failed_if_possible();
            }

            return false;
        }
    }

    void win_pipe_create_operation::cancel(bool by_timeout) noexcept
    {
        by_timeout ? set_timeout_if_possible() : set_canceled_if_possible();

        auto pipe = m_pipe.lock();
        if(!pipe->empty()) {
            ::CancelIo(pipe->get());
        }
    }

    void win_pipe_create_operation::exec(const std::function<void(win::pipe_handle::handle_type)> &f)
    {
        f(m_pipe.lock()->get());
    }

    void win_pipe_create_operation::resolve()
    {
        auto res = m_res.load(std::memory_order_acquire);
        assert(res != win_pipe_operation_res::unknown);

        auto pipe = m_pipe.lock();
        if(res == win_pipe_operation_res::success) {
            assert(!pipe->empty());
            m_promise.set_value(cl::ftuple(res, std::move(*pipe)));
        } else {
            pipe->reset();
            m_promise.set_value(cl::ftuple(res, win::pipe_handle{}));
        }
    }

    win_pipe_create_operation::future win_pipe_create_operation::get_future()
    {
        return m_promise.get_future();
    }

    void win_pipe_create_operation::set_success() noexcept
    {
        m_res.store(win_pipe_operation_res::success, std::memory_order_release);
    }

    void win_pipe_create_operation::set_canceled_if_possible() noexcept
    {
        auto expected = win_pipe_operation_res::unknown;
        m_res.compare_exchange_strong(expected,
                                      win_pipe_operation_res::canceled,
                                      std::memory_order_release,
                                      std::memory_order_relaxed);
    }

    void win_pipe_create_operation::set_timeout_if_possible() noexcept
    {
        auto expected = win_pipe_operation_res::unknown;
        m_res.compare_exchange_strong(expected,
                                      win_pipe_operation_res::timeout,
                                      std::memory_order_release,
                                      std::memory_order_relaxed);
    }

    void win_pipe_create_operation::set_failed_if_possible() noexcept
    {
        auto expected = win_pipe_operation_res::unknown;
        m_res.compare_exchange_strong(expected,
                                      win_pipe_operation_res::failed,
                                      std::memory_order_release,
                                      std::memory_order_relaxed);
    }
}

#endif


