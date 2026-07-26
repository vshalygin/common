#ifdef _WIN32
#include "win-pipe-iocp-owner.h"

#include <algorithm>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

namespace vshalygin::rpc::internal {
    namespace {
        std::wstring make_full_pipe_name(const std::wstring &pipe_name)
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
        const auto full_pipe_name = make_full_pipe_name(pipe_name);
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
            m_iocp.associate(pipe.get(),
                             static_cast<ULONG_PTR>(win_pipe_iocp_key::process_operation),
                             ec);
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
                //will be resolved later in iocp worker thread
            } else {
                overlapped->resolve(false, ::GetLastError());
            }
        });
    }

    win::pipe_handle win_pipe_iocp_owner::open_pipe(const std::wstring &pipe_name,
                                                    std::chrono::milliseconds timeout)
    {
        using clock = std::chrono::steady_clock;

        const auto full_pipe_name = make_full_pipe_name(pipe_name);
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
                m_iocp.associate(pipe.get(),
                                 static_cast<ULONG_PTR>(win_pipe_iocp_key::process_operation));
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

    void win_pipe_iocp_owner::read_async(win_pipe_read_operation *overlapped)
    {
        m_iocp_thread.post([overlapped]() {
            if(overlapped->get_result() != win_pipe_operation_res::unknown) {
                overlapped->resolve();
                return;
            }

            std::error_code ec;
            overlapped->read(ec);

            if(ec) {
                overlapped->set_failed_if_possible();
                overlapped->resolve();
            }
        });
    }

    void win_pipe_iocp_owner::cancel_read(win_pipe_read_operation *overlapped)
    {
        m_iocp_thread.post([overlapped]() {
            overlapped->cancel();
        });
    }

    void win_pipe_iocp_owner::write_async(win_pipe_write_operation *overlapped)
    {
        m_iocp_thread.post([overlapped]() {
            if(overlapped->get_result() != win_pipe_operation_res::unknown) {
                overlapped->resolve();
                return;
            }

            std::error_code ec;
            overlapped->write(ec);

            if(ec) {
                overlapped->set_failed_if_possible();
                overlapped->resolve();
            }
        });
    }

    void win_pipe_iocp_owner::cancel_write(win_pipe_write_operation *overlapped)
    {
        m_iocp_thread.post([overlapped]() {
            overlapped->cancel();
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
            auto key = static_cast<win_pipe_iocp_key>(status.key);
            if(key == win_pipe_iocp_key::interrupt_iocp) {
                return;
            }

            assert(key == win_pipe_iocp_key::process_operation);
            assert(status.overlapped);
            const auto op = reinterpret_cast<win_pipe_operation *>(status.overlapped);
            switch(op->kind) {
                case win_pipe_operation_kind::create:
                {
                    auto create_op = reinterpret_cast<win_pipe_create_operation *>(op);
                    create_op->resolve(status.success, status.error);
                    break;
                }
                case win_pipe_operation_kind::write:
                {
                    auto write_operation = reinterpret_cast<win_pipe_write_operation *>(op);
                    if(status.success) {
                        write_operation->set_success();
                    } else {
                        write_operation->set_failed_if_possible();
                    }
                    write_operation->resolve();
                    break;
                }
                case win_pipe_operation_kind::read:
                {
                    auto read_operation = reinterpret_cast<win_pipe_read_operation *>(op);
                    read_operation->add_read_bytes(status.bytes_transferred);
                    if(status.success) {
                        read_operation->set_success();
                        read_operation->resolve();
                    } else if(status.error == ERROR_MORE_DATA) {
                        read_operation->add_buffer_chunk();
                        read_async(read_operation);
                    } else {
                        read_operation->set_failed_if_possible();
                        read_operation->resolve();
                    }
                    break;
                }
                default:
                    assert(!"unknown win_pipe_operation_kind");
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
