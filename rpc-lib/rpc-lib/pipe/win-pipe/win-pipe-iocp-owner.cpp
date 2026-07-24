#ifdef _WIN32
#include "win-pipe-iocp-owner.h"

#include <algorithm>

namespace vshalygin::rpc::internal {
    namespace {
        std::wstring make_fule_pipe_name(const std::wstring &pipe_name)
        {
            return L"\\\\.\\pipe\\" + pipe_name;
        }
    }

    win_pipe_iocp_owner::win_pipe_iocp_owner()
        : m_worker([this]() { run_worker_loop(); })
    {}
    
    win_pipe_iocp_owner::~win_pipe_iocp_owner()
    {
        interrupt_worker_loop();
        m_worker.join();

        m_iocp_thread.stop();
    }
    
    void win_pipe_iocp_owner::create_pipe_async(const std::wstring &pipe_name,
                                                win_pipe_create_operation *overlapped)
    {
        const auto full_pipe_name = make_fule_pipe_name(pipe_name);
        win::pipe_handle pipe(
            ::CreateNamedPipeW(full_pipe_name.c_str(),
                               PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                               PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                               PIPE_UNLIMITED_INSTANCES,
                               8192,
                               8192,
                               0,
                               NULL));
        if(!pipe) {
            overlapped->resolve(false, ::GetLastError());
            return;
        }

        m_iocp_thread.post([pipe = std::move(pipe), overlapped, this]() mutable {
            std::error_code ec;
            m_iocp.associate(pipe.get(), static_cast<ULONG_PTR>(win_pipe_iocp_key::connect_pipe), ec);
            if(ec) {
                overlapped->resolve(false, static_cast<DWORD>(ec.value()));
                return;
            }

            auto pipe_ref = pipe.get();
            overlapped->set_pipe(std::move(pipe));
            auto r = ::ConnectNamedPipe(pipe_ref, reinterpret_cast<OVERLAPPED *>(overlapped));
            if(r || ::GetLastError() == ERROR_PIPE_CONNECTED) {
                overlapped->resolve(true, static_cast<DWORD>(ERROR_SUCCESS));
            } else if(::GetLastError() == ERROR_IO_PENDING) {
                //will be resolved later
            } else {
                overlapped->resolve(false, ::GetLastError());
            }
        });
    }

    win::pipe_handle win_pipe_iocp_owner::open_pipe(const std::wstring &pipe_name,
                                                    std::chrono::milliseconds timeout)
    {
        using clock = std::chrono::steady_clock;

        const auto full_pipe_name = make_fule_pipe_name(pipe_name);
        const auto start = clock::now();
        const auto deadline = (clock::time_point::max() - start) < timeout ?
                               clock::time_point::max() :
                               start + timeout;
        while(true)
        {
            win::pipe_handle pipe(::CreateFileW(full_pipe_name.c_str(),
                                                GENERIC_READ | GENERIC_WRITE,
                                                0,
                                                nullptr,
                                                OPEN_EXISTING,
                                                FILE_FLAG_OVERLAPPED,
                                                nullptr));

            if(pipe) {
                m_iocp.associate(pipe.get(), static_cast<ULONG_PTR>(win_pipe_iocp_key::read_write));
                return pipe;
            }

            if(::GetLastError() != ERROR_PIPE_BUSY) {
                throw std::system_error(::GetLastError(), std::system_category());
            }

            auto now = clock::now();
            const auto t = std::max((deadline - now).count(), 0ll);
            const DWORD tt = static_cast<DWORD>(std::min(t, static_cast<decltype(t)>(MAXDWORD)));
            if(!::WaitNamedPipeW(full_pipe_name.c_str(), tt)) {
                throw std::system_error(::GetLastError(), std::system_category());
            }
        }
    }

    void win_pipe_iocp_owner::cancel_create(win_pipe_create_operation *overlapped)
    {
        m_iocp_thread.post([overlapped]() {
            overlapped->cancel();
        });
    }

    void win_pipe_iocp_owner::run_worker_loop()
    {
        while(true) {
            const auto status = m_iocp.get();
            switch(static_cast<win_pipe_iocp_key>(status.key)) {
                case win_pipe_iocp_key::interrupt_iocp:
                    return;
                case win_pipe_iocp_key::connect_pipe:
                {
                    assert(status.overlapped);
                    auto op = reinterpret_cast<win_pipe_create_operation *>(status.overlapped);
                    op->resolve(status.success, status.error);
                    break;
                }
                default:
                    assert(false); //TODO temp
            }
            
        }
    }

    void win_pipe_iocp_owner::interrupt_worker_loop() noexcept
    {
        std::error_code ec;
        m_iocp.post(0, static_cast<ULONG_PTR>(win_pipe_iocp_key::interrupt_iocp), nullptr, ec);
        assert(!ec);
    }
}

#endif
