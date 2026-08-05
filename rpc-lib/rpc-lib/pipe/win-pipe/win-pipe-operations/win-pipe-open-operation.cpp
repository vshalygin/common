#ifdef _WIN32
#include "win-pipe-open-operation.h"

namespace vshalygin::rpc::internal {
    win_pipe_open_operation::win_pipe_open_operation(const std::wstring &pipe_name,
                                                     cl::thread_pool *thread_pool)
        : m_full_pipe_name(L"\\\\.\\pipe\\" + pipe_name)
        , m_promise(thread_pool,
                    [](win_pipe_operation_res r, win::pipe_handle &&p) { return ftuple(r, std::move(p)); })
    {}

    win_pipe_open_operation::~win_pipe_open_operation()
    {
        cancel();
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

    void win_pipe_open_operation::cancel() noexcept
    {
        m_canceled_event.set();
    }

    future<ftuple<win_pipe_operation_res, win::pipe_handle>> win_pipe_open_operation::get_future()
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
                m_promise.resolve(win_pipe_operation_res::success, std::move(pipe));
                break;
            }

            if(::GetLastError() != ERROR_PIPE_BUSY && ::GetLastError() != ERROR_FILE_NOT_FOUND) {
                m_promise.resolve(win_pipe_operation_res::failed, std::move(pipe));
                break;
            }

            auto r = m_canceled_event.wait_for(std::chrono::milliseconds(100));
            if(r) {
                m_promise.resolve(win_pipe_operation_res::canceled, std::move(pipe));
                break;
            }
        }
    }
}

#endif
