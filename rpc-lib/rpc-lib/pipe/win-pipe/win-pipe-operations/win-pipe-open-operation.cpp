#ifdef _WIN32
#include "win-pipe-open-operation.h"

namespace vshalygin::rpc::internal {
    win_pipe_open_operation::win_pipe_open_operation(const std::wstring &pipe_name,
                                                     cl::thread_pool *thread_pool)
        : m_full_pipe_name(L"\\\\.\\pipe\\" + pipe_name)
        , m_promise(thread_pool,
                    [](win_pipe_operation_res r, win::pipe_handle &&p) { return cl::ftuple(r, std::move(p)); })
    {}

    win_pipe_open_operation::~win_pipe_open_operation()
    {
        cancel(false);
        if(m_thread.joinable()) {
            m_thread.join();
        }
    }

    void win_pipe_open_operation::start()
    {
        assert(!m_thread.joinable());

        m_thread = std::thread([this]() mutable {
            do_openning();
        });
    }

    void win_pipe_open_operation::cancel(bool by_timeout) noexcept
    {
        {
            std::lock_guard g(m_cancel_mtx);
            if(m_cancel_event == cancel_event::none) {
                m_cancel_event = by_timeout ? cancel_event::canceled_by_timeout : cancel_event::canceled;
            }
        }

        m_cancel_cv.notify_all();
    }

    cl::future<cl::thread_pool, cl::ftuple<win_pipe_operation_res, win::pipe_handle>> win_pipe_open_operation::get_future()
    {
        return m_promise.get_future();
    }

    void win_pipe_open_operation::do_openning()
    {
        while(true)
        {
            win::pipe_handle pipe(::CreateFileW(m_full_pipe_name.c_str(),
                                                GENERIC_READ | GENERIC_WRITE,
                                                0,
                                                nullptr,
                                                OPEN_EXISTING,
                                                FILE_FLAG_OVERLAPPED,
                                                nullptr));

            if(pipe) {
                DWORD mode = PIPE_READMODE_MESSAGE;

                if(!::SetNamedPipeHandleState(pipe.get(), &mode, nullptr, nullptr)) {
                    m_promise.resolve(win_pipe_operation_res::failed, std::move(pipe));
                    break;
                }

                m_promise.resolve(win_pipe_operation_res::success, std::move(pipe));
                break;
            }

            if(::GetLastError() != ERROR_PIPE_BUSY && ::GetLastError() != ERROR_FILE_NOT_FOUND) {
                m_promise.resolve(win_pipe_operation_res::failed, std::move(pipe));
                break;
            }

            std::unique_lock lock(m_cancel_mtx);
            auto r = m_cancel_cv.wait_for(
                lock, std::chrono::milliseconds(100), [this] { return m_cancel_event != cancel_event::none; });
            if(r) {
                auto res = (m_cancel_event == cancel_event::canceled_by_timeout) ?
                                               win_pipe_operation_res::timeout :
                                               win_pipe_operation_res::canceled;
                m_promise.resolve(res, std::move(pipe));
                break;
            }
        }
    }
}

#endif
