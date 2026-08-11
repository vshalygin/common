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

        pipe_wait_res to_pipe_wait_res(win_pipe_operation_res r)
        {
            switch(r) {
                case win_pipe_operation_res::success:
                    return pipe_wait_res::success;
                case win_pipe_operation_res::canceled:
                    return pipe_wait_res::canceled;
                case win_pipe_operation_res::timeout:
                    return pipe_wait_res::timeout;
                case win_pipe_operation_res::failed:
                    return pipe_wait_res::failed;
                default:
                    assert(!"unexpected win_pipe_operation_res");
                    return pipe_wait_res::failed;
            }
        }
    }

    std::shared_ptr<win_pipe_iocp_owner> win_pipe_iocp_owner::create()
    {
        return std::shared_ptr<win_pipe_iocp_owner>(new win_pipe_iocp_owner);
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

    future<ftuple<pipe_wait_res, win::pipe_handle>> win_pipe_iocp_owner::open_pipe_async(
        win_pipe_open_operation *op)
    {
        auto f = op->get_future();
        op->start();

        return f.then([s = shared_from_this()](win_pipe_operation_res r, win::pipe_handle &&p) mutable {
                          if(win_pipe_operation_res::success == r) {
                              s->m_iocp.associate(
                                  p.get(),
                                  static_cast<ULONG_PTR>(win_pipe_iocp_key::process_operation));
                          }
                          return ftuple(to_pipe_wait_res(r), std::move(p));
                      });
    }

    //Generally speaking this method is pointless. Added just for symmetry
    void win_pipe_iocp_owner::cancel_open_pipe(win_pipe_open_operation *op)
    {
        op->cancel(false);
    }

    void win_pipe_iocp_owner::read_async(win_pipe_read_operation *overlapped)
    {
        m_iocp_thread.post([overlapped]() {
            if(overlapped->get_result() != win_pipe_operation_res::unknown) {
                overlapped->resolve();
                return;
            }

            std::error_code ec;
            overlapped->start(ec);

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
            overlapped->start(ec);

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
            const auto op = reinterpret_cast<win_pipe_overlapped *>(status.overlapped);
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
