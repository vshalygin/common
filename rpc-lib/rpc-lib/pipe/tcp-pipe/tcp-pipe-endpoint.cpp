#include "tcp-pipe-endpoint.h"

#include <common-lib/synchronization/ordered-lock.h>
#include <common-lib/synchronization/ordered-mutex.h>

#include <boost/endian/conversion.hpp>
#include <boost/asio/write.hpp>

#include <mutex>
#include <vector>
#include <list>
#include <array>

namespace vshalygin::rpc {
    using write_future = tcp_pipe_endpoint::write_future;
    using write_promise = promise<pipe_op_res, pipe_op_res>;
    using read_future = tcp_pipe_endpoint::read_future;

    namespace {
        static constexpr size_t s_header_size = sizeof(uint32_t);

        class write_operation
        {
        public:
            write_operation(cl::buffer &&message, write_promise &&promise)
                : m_message(std::move(message))
                , m_write_header_big_endian(boost::endian::native_to_big(static_cast<uint32_t>(m_message.size())))
                , m_bytes_total(s_header_size + m_message.size())
                , m_promise(std::move(promise))
            {}

            write_operation(const write_operation &) = delete;
            write_operation &operator=(const write_operation &) = delete;

            void add_transfered_bytes(size_t bytes) noexcept
            {
                m_bytes_transferred += bytes;
            }

            void resolve_promise_once(pipe_op_res r)
            {
                if(!m_is_promise_resolved) {
                    m_is_promise_resolved = true;
                    m_promise.resolve(r);
                }
            }

            auto get_unwritten_header_buffer() const noexcept
            {
                assert(m_bytes_transferred <= s_header_size);
                auto b = reinterpret_cast<const std::byte *>(&m_write_header_big_endian);
                return boost::asio::buffer(b + m_bytes_transferred,
                                           s_header_size - m_bytes_transferred);
            }

            auto get_unwritten_payload_buffer() const noexcept
            {
                assert(m_bytes_transferred >= s_header_size);
                assert(m_bytes_transferred <= m_bytes_total);

                const auto buffer_offset = m_bytes_transferred - s_header_size;

                return boost::asio::buffer(m_message.data() + buffer_offset, m_message.size() - buffer_offset);
            }

            bool is_header_write_completed() const noexcept
            {
                return m_bytes_transferred >= s_header_size;
            }

            bool is_payload_write_completed() const noexcept
            {
                assert(m_bytes_transferred <= m_bytes_total);

                return m_bytes_transferred == m_bytes_total;
            }

        private:
            cl::buffer m_message;
            uint32_t m_write_header_big_endian = 0;
            size_t m_bytes_transferred = 0;
            size_t m_bytes_total = 0;

            bool m_is_promise_resolved = false;
            write_promise m_promise;
        };
    }

    class tcp_pipe_endpoint::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        impl(std::shared_ptr<cl::thread_pool> thread_pool,
             socket &&socket);

        impl(const tcp_pipe_endpoint &) = delete;
        impl &operator=(const impl &) = delete;

        bool is_connected() const;
        void set_disconnect_callback(cl::thread_pool_task<void()> &&callback);

        write_future write_async(cl::buffer &&msg);
        read_future read_async();
        write_future write_async(cl::buffer &&msg, std::chrono::milliseconds timeout);
        read_future read_async(std::chrono::milliseconds timeout);

        void invalidate();

    private:
        void invalidate_unsafe(bool by_cancel);

        void start_write_header_async();
        void start_write_payload_async();

        void on_write_header(const boost::system::error_code &ec,
                             std::size_t bytes_transferred);
        void on_write_payload(const boost::system::error_code &ec,
                              std::size_t bytes_transferred);

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;

        mutable cl::ordered_mutex<1> m_write_mtx;
        std::list<write_operation> m_write_operations;

        mutable cl::ordered_mutex<2> m_socket_mtx;
        socket m_socket;
        bool m_was_invalidated_by_fail = false;
        std::vector<cl::thread_pool_task<void()>> m_disconnect_callbacks;

    };

    tcp_pipe_endpoint::impl::impl(std::shared_ptr<cl::thread_pool> thread_pool,
                                  socket &&socket)
        : m_thread_pool(std::move(thread_pool))
        , m_socket(std::move(socket))
    {
        assert(m_socket.is_open());
    }

    bool tcp_pipe_endpoint::impl::is_connected() const
    {
        cl::ordered_lock guard(m_socket_mtx);
        return m_socket.is_open();
    }

    void tcp_pipe_endpoint::impl::set_disconnect_callback(cl::thread_pool_task<void()> &&callback)
    {
        cl::ordered_lock guard(m_socket_mtx);
        if(m_socket.is_open()) {
            m_disconnect_callbacks.push_back(std::move(callback));
        } else {
            callback.exec();
        }
    }

    write_future tcp_pipe_endpoint::impl::write_async(cl::buffer &&msg)
    {
        assert(msg.size() != 0);

        auto promise = make_promise(m_thread_pool.get(), [](pipe_op_res r) { return r; });
        auto future = promise.get_future();

        cl::ordered_lock guard(m_write_mtx, m_socket_mtx);
        if(!m_socket.is_open()) {
            promise.resolve(pipe_op_res::failed);
            return future;
        }

        m_write_operations.emplace_front(std::move(msg), std::move(promise));

        if(m_write_operations.size() == 1) {
            start_write_header_async();
        }
        return future;
    }

    void tcp_pipe_endpoint::impl::start_write_header_async()
    {
        assert(!m_write_operations.empty());

        auto &op = m_write_operations.back();

        auto self = shared_from_this();
        m_socket.async_write_some(op.get_unwritten_header_buffer(),
                                  [self](const boost::system::error_code &ec,
                                  std::size_t bytes_transferred) mutable {
                                      self->on_write_header(ec, bytes_transferred);
                                  });
    }

    void tcp_pipe_endpoint::impl::start_write_payload_async()
    {
        assert(!m_write_operations.empty());

        auto &op = m_write_operations.back();

        auto self = shared_from_this();
        m_socket.async_write_some(op.get_unwritten_payload_buffer(),
                                  [self](const boost::system::error_code &ec,
                                  std::size_t bytes_transferred) mutable {
                                      self->on_write_payload(ec, bytes_transferred);
                                  });
    }

    void tcp_pipe_endpoint::impl::on_write_header(const boost::system::error_code &ec,
                                                  std::size_t bytes_transferred)
    {
        cl::ordered_lock guard1(m_write_mtx);

        assert(!m_write_operations.empty());

        auto &op = m_write_operations.back();
        op.add_transfered_bytes(bytes_transferred);
        if(ec) {
            if(ec == boost::asio::error::operation_aborted) {
                cl::ordered_lock guard2 = push_back(std::move(guard1), m_socket_mtx);
                bool is_socket_open = m_socket.is_open();
                op.resolve_promise_once(is_socket_open ? pipe_op_res::timeout :
                                        m_was_invalidated_by_fail ? pipe_op_res::failed : pipe_op_res::canceled);

                if(is_socket_open) {
                    if(!op.is_header_write_completed()) {
                        start_write_header_async();
                    } else {
                        start_write_payload_async();
                    }
                } else {
                    m_write_operations.pop_back();
                    while(!m_write_operations.empty()) {
                        m_write_operations.back().resolve_promise_once(m_was_invalidated_by_fail ?
                                                                       pipe_op_res::failed :
                                                                       pipe_op_res::canceled);
                        m_write_operations.pop_back();
                    }
                }
            } else {
                cl::ordered_lock guard2 = push_back(std::move(guard1), m_socket_mtx);
                invalidate_unsafe(false);

                while(!m_write_operations.empty()) {
                    m_write_operations.back().resolve_promise_once(pipe_op_res::failed);
                    m_write_operations.pop_back();
                }
            }
        } else {
            cl::ordered_lock guard2 = push_back(std::move(guard1), m_socket_mtx);
            if(!op.is_header_write_completed()) {
                start_write_header_async();
            } else {
                start_write_payload_async();
            }
        }
    }

    void tcp_pipe_endpoint::impl::on_write_payload(const boost::system::error_code &ec,
                                                   std::size_t bytes_transferred)
    {
        cl::ordered_lock guard1(m_write_mtx);
        assert(!m_write_operations.empty());
        auto &op = m_write_operations.back();
        op.add_transfered_bytes(bytes_transferred);

        if(ec) {
            if(ec == boost::asio::error::operation_aborted) {
                cl::ordered_lock guard2 = push_back(std::move(guard1), m_socket_mtx);
                bool is_socket_open = m_socket.is_open();
                op.resolve_promise_once(is_socket_open ? pipe_op_res::timeout :
                                        m_was_invalidated_by_fail ? pipe_op_res::failed : pipe_op_res::canceled);

                if(is_socket_open) {
                    if(!op.is_payload_write_completed()) {
                        start_write_payload_async();
                    } else {
                        m_write_operations.pop_back();
                        if(!m_write_operations.empty()) {
                            start_write_header_async();
                        }
                    }
                } else {
                    m_write_operations.pop_back();
                    while(!m_write_operations.empty()) {
                        m_write_operations.back().resolve_promise_once(m_was_invalidated_by_fail ?
                                                                       pipe_op_res::failed :
                                                                       pipe_op_res::canceled);
                        m_write_operations.pop_back();
                    }
                }

            } else {
                cl::ordered_lock guard2 = push_back(std::move(guard1), m_socket_mtx);
                invalidate_unsafe(false);

                while(!m_write_operations.empty()) {
                    m_write_operations.back().resolve_promise_once(pipe_op_res::failed);
                    m_write_operations.pop_back();
                }
            }
        } else {
            cl::ordered_lock guard2 = push_back(std::move(guard1), m_socket_mtx);
            if(!op.is_payload_write_completed()) {
                start_write_payload_async();
            } else {
                op.resolve_promise_once(pipe_op_res::success);

                m_write_operations.pop_back();
                if(!m_write_operations.empty()) {
                    start_write_header_async();
                }
            }
        }
    }

    read_future tcp_pipe_endpoint::impl::read_async()
    {
        //TODO add implementaion
        return {};
    }

    write_future tcp_pipe_endpoint::impl::write_async(cl::buffer && /*msg*/, std::chrono::milliseconds /*timeout*/)
    {
        //TODO add implementaion
        return {};
    }

    read_future tcp_pipe_endpoint::impl::read_async(std::chrono::milliseconds /*timeout*/)
    {
        //TODO add implementaion
        return {};
    }

    void tcp_pipe_endpoint::impl::invalidate()
    {
        cl::ordered_lock guard(m_write_mtx, m_socket_mtx);
        invalidate_unsafe(true);
    }

    void tcp_pipe_endpoint::impl::invalidate_unsafe(bool by_cancel)
    {
        if(m_socket.is_open()) {
            m_was_invalidated_by_fail = !by_cancel;

            boost::system::error_code ignored;
            m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored);
            m_socket.close(ignored);
        }

        for(auto &cb : m_disconnect_callbacks) {
            cb.exec();
        }
        m_disconnect_callbacks.clear();
    }

    tcp_pipe_endpoint::tcp_pipe_endpoint(std::shared_ptr<cl::thread_pool> thread_pool,
                                         socket &&socket)
        : m_impl(std::make_shared<impl>(std::move(thread_pool), std::move(socket)))
    {}

    tcp_pipe_endpoint::~tcp_pipe_endpoint()
    {
        invalidate();
    }

    bool tcp_pipe_endpoint::is_connected() const
    {
        return m_impl->is_connected();
    }

    void tcp_pipe_endpoint::set_disconnect_callback(cl::thread_pool_task<void()> &&callback)
    {
        m_impl->set_disconnect_callback(std::move(callback));
    }

    write_future tcp_pipe_endpoint::write_async(cl::buffer &&msg)
    {
        return m_impl->write_async(std::move(msg));
    }

    read_future tcp_pipe_endpoint::read_async()
    {
        return m_impl->read_async();
    }

    write_future tcp_pipe_endpoint::write_async(cl::buffer &&msg, std::chrono::milliseconds timeout)
    {
        return m_impl->write_async(std::move(msg), timeout);
    }

    read_future tcp_pipe_endpoint::read_async(std::chrono::milliseconds timeout)
    {
        return m_impl->read_async(timeout);
    }

    void tcp_pipe_endpoint::invalidate()
    {
        m_impl->invalidate();
    }
}
