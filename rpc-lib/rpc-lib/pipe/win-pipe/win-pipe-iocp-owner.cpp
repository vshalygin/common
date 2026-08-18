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

        m_read_iocp_thread.stop();
        m_write_iocp_thread.stop();
    }
    
    future<ftuple<pipe_wait_res, win::pipe_handle>> win_pipe_iocp_owner::create_pipe_async(
        std::shared_ptr<win_pipe_create_operation> overlapped)
    {
        auto f = overlapped->get_future();

        m_read_iocp_thread.post([overlapped = std::move(overlapped), this]() mutable {
            if(!overlapped->create_pipe()) {
                overlapped->resolve();
                return;
            }

            std::error_code ec;
            overlapped->exec(
                [&ec, this](win::pipe_handle::handle_type p) mutable {
                    m_iocp.associate(p,
                                     static_cast<ULONG_PTR>(win_pipe_iocp_key::process_operation),
                                     ec);
                });

            if(ec) {
                overlapped->set_failed_if_possible();
                overlapped->resolve();
                return;
            }

            if(!overlapped->start_wait_connect()) {
                overlapped->resolve();
            }
        });

        return f.then([](win_pipe_operation_res r, win::pipe_handle &&p) mutable {
                          return ftuple(to_pipe_wait_res(r), std::move(p));
                      });
    }

    future<ftuple<pipe_wait_res, win::pipe_handle>> win_pipe_iocp_owner::open_pipe_async(win_pipe_open_operation *op)
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

    void win_pipe_iocp_owner::read_async(std::shared_ptr<win_pipe_read_operation> overlapped)
    {
        m_read_iocp_thread.post([overlapped]() {
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

    void win_pipe_iocp_owner::cancel_read(std::shared_ptr<win_pipe_read_operation> overlapped)
    {
        m_read_iocp_thread.post([overlapped]() {
            overlapped->cancel();
        });
    }

    void win_pipe_iocp_owner::write_async(std::shared_ptr<win_pipe_write_operation> overlapped)
    {
        m_write_iocp_thread.post([overlapped]() {
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

    void win_pipe_iocp_owner::cancel_write(std::shared_ptr<win_pipe_write_operation> overlapped)
    {
        m_write_iocp_thread.post([overlapped]() {
            overlapped->cancel();
        });
    }

    void win_pipe_iocp_owner::cancel_create(
        std::shared_ptr<win_pipe_create_operation> overlapped, bool by_timeout)
    {
        m_read_iocp_thread.post([overlapped = std::move(overlapped), by_timeout]() {
            overlapped->cancel(by_timeout);
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
                    auto create_op = reinterpret_cast<win_pipe_create_operation *>(op)->shared_from_this();
                    if(status.success) {
                        create_op->set_success();
                    } else {
                        create_op->set_failed_if_possible();
                    }

                    create_op->resolve();
                    break;
                }
                case win_pipe_operation_kind::write:
                {
                    auto write_operation = reinterpret_cast<win_pipe_write_operation *>(op)->shared_from_this();
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
                    auto read_operation = reinterpret_cast<win_pipe_read_operation *>(op)->shared_from_this();
                    read_operation->add_read_bytes(status.bytes_transferred);
                    if(status.success) {
                        read_operation->set_success();
                        read_operation->resolve();
                    } else if(status.error == ERROR_MORE_DATA && read_operation->add_buffer_chunk()) {
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
