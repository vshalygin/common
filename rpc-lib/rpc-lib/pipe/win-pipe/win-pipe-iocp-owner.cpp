#ifdef _WIN32
#include "win-pipe-iocp-owner.h"

namespace vshalygin::rpc::internal {
    win_pipe_iocp_owner::win_pipe_iocp_owner(const std::wstring &pipe_name)
        : m_full_pipe_name(L"\\\\.\\pipe\\" + pipe_name)
        , m_worker([this]() { run_worker_loop(); })
    {}
    
    win_pipe_iocp_owner::~win_pipe_iocp_owner()
    {
        interrupt_worker_loop();
        m_worker.join();

        m_iocp_thread.stop();
    }
    
    void win_pipe_iocp_owner::create_pipe_async(win_pipe_create_operation *overlapped)
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
