#include "tcp-pipe-endpoint.h"
#include <rpc-lib/consts.h>

#include <common-lib/synchronization/ordered-lock.h>
#include <common-lib/synchronization/ordered-mutex.h>
#include <common-lib/timer/multiple-timer.h>

#include <boost/endian/conversion.hpp>
#include <boost/asio/write.hpp>

#include <mutex>
#include <vector>
#include <list>
#include <array>
#include <optional>
#include <algorithm>

namespace vshalygin::rpc {
    using write_future = tcp_pipe_endpoint::write_future;
    using write_promise = cl::promise<cl::thread_pool, pipe_op_res(pipe_op_res)>;
    using read_future = tcp_pipe_endpoint::read_future;
    using read_promise = cl::promise<cl::thread_pool, cl::ftuple<pipe_op_res, cl::buffer>(pipe_op_res, cl::buffer &&)>;

    namespace {
        static constexpr size_t s_header_size = sizeof(uint32_t);

        class write_operation
        {
        public:
            write_operation(cl::buffer &&message, write_promise &&promise)
                : m_payload_buffer(std::move(message))
                , m_write_header_big_endian(boost::endian::native_to_big(static_cast<uint32_t>(m_payload_buffer.size())))
                , m_bytes_total(s_header_size + m_payload_buffer.size())
                , m_promise(std::move(promise))
            {}

            write_operation(const write_operation &) = delete;
            write_operation &operator=(const write_operation &) = delete;

            void add_bytes(size_t bytes) noexcept
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

                return boost::asio::buffer(m_payload_buffer.data() + buffer_offset,
                                           m_payload_buffer.size() - buffer_offset);
            }

            bool is_header_completed() const noexcept
            {
                return m_bytes_transferred >= s_header_size;
            }

            bool is_payload_completed() const noexcept
            {
                assert(m_bytes_transferred <= m_bytes_total);

                return m_bytes_transferred == m_bytes_total;
            }

        private:
            cl::buffer m_payload_buffer;
            uint32_t m_write_header_big_endian = 0;
            size_t m_bytes_transferred = 0;
            size_t m_bytes_total = 0;

            bool m_is_promise_resolved = false;
            write_promise m_promise;
        };

        class read_operation
        {
        public:
            read_operation(read_promise &&promise)
                : m_promise(std::move(promise))
            {}

            read_operation(const read_operation &) = delete;
            read_operation &operator=(const read_operation &) = delete;

            void add_bytes(size_t bytes) noexcept
            {
                m_bytes_read += bytes;
                if(!m_payload_buffer && m_bytes_read == s_header_size) {
                    auto message_size = boost::endian::big_to_native(m_header);
                    if(message_size != 0 && message_size <= MaxTransferMessageSize) {
                        m_payload_buffer = cl::buffer(message_size);
                    }
                }
            }

            bool is_payload_buffer_valid() const noexcept
            {
                return static_cast<bool>(m_payload_buffer);
            }

            void resolve_promise_once(pipe_op_res r)
            {
                if(!m_is_promise_resolved) {
                    m_is_promise_resolved = true;
                    if(r == pipe_op_res::success) {
                        m_promise.resolve(r, std::move(m_payload_buffer));
                    } else {
                        m_promise.resolve(r, {});
                    }
                }
            }

            auto get_buffer_for_unread_header() noexcept
            {
                assert(m_bytes_read <= s_header_size);
                auto b = reinterpret_cast<std::byte *>(&m_header);
                return boost::asio::buffer(b + m_bytes_read,
                                           s_header_size - m_bytes_read);
            }

            auto get_buffer_for_unread_payload() noexcept
            {
                assert(m_bytes_read >= s_header_size);
                assert(m_payload_buffer && m_bytes_read <= m_payload_buffer.size() + s_header_size);

                const auto buffer_offset = m_bytes_read - s_header_size;

                return boost::asio::buffer(m_payload_buffer.data() + buffer_offset,
                                           m_payload_buffer.size() - buffer_offset);
            }

            bool is_header_completed() const noexcept
            {
                return m_bytes_read >= s_header_size;
            }

            bool is_payload_completed() const noexcept
            {
                return m_payload_buffer && m_bytes_read == m_payload_buffer.size() + s_header_size;
            }

        private:
            uint32_t m_header = 0;
            cl::buffer m_payload_buffer;
            size_t m_bytes_read = 0;

            bool m_is_promise_resolved = false;
            read_promise m_promise;
        };
    }

    class tcp_pipe_endpoint::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        impl(cl::thread_pool *thread_pool,
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
        write_future write_async(cl::buffer &&msg, const std::optional<std::chrono::milliseconds> &timeout);
        read_future read_async(const std::optional<std::chrono::milliseconds> &timeout);

        void invalidate_unsafe(bool by_cancel);

        void start_write_header_async();
        void start_write_payload_async();

        void start_read_header_async();
        void start_read_payload_async();

        template<typename Operations, typename OrderedMutex, typename StartHeaderOpAsync, typename StartPayloadOpAsync>
        void on_header_operation(const boost::system::error_code &ec,
                                 std::size_t bytes,
                                 Operations &operations,
                                 OrderedMutex &op_mutex,
                                 StartHeaderOpAsync start_header_op_async,
                                 StartPayloadOpAsync start_payload_op_async);

        template<typename Operations, typename OrderedMutex, typename StartHeaderOpAsync, typename StartPayloadOpAsync>
        void on_payload_operation(const boost::system::error_code &ec,
                                  std::size_t bytes,
                                  Operations &operations,
                                  OrderedMutex &op_mutex,
                                  StartHeaderOpAsync start_header_op_async,
                                  StartPayloadOpAsync start_payload_op_async);

        template<typename Operations>
        void complete_all_operations_unsafe(pipe_op_res r, Operations &operations);

    private:
        cl::thread_pool *m_thread_pool;

        mutable cl::ordered_mutex<1> m_write_mtx;
        uint64_t m_next_write_op_id = 0;
        struct write_operation_data
        {
            write_operation_data(cl::buffer &&message, write_promise &&promise,
                                 uint64_t id_, std::optional<uint64_t> timer_id_)
                : op(std::move(message), std::move(promise))
                , id(id_)
                , timer_id(timer_id_)
            {}

            uint64_t id;
            write_operation op;
            std::optional<uint64_t> timer_id;
        };
        std::list<write_operation_data> m_write_operations;

        mutable cl::ordered_mutex<2> m_read_mtx;
        uint64_t m_next_read_op_id = 0;
        struct read_operation_data
        {
            read_operation_data(read_promise &&promise,
                                uint64_t id_, std::optional<uint64_t> timer_id_)
                : op(std::move(promise))
                , id(id_)
                , timer_id(timer_id_)
            {}

            uint64_t id;
            read_operation op;
            std::optional<uint64_t> timer_id;
        };
        std::list<read_operation_data> m_read_operations;

        mutable cl::ordered_mutex<3> m_socket_mtx;
        socket m_socket;
        bool m_was_invalidated_by_fail = false;
        std::vector<cl::thread_pool_task<void()>> m_disconnect_callbacks;

        cl::multiple_timer m_timer;
    };

    tcp_pipe_endpoint::impl::impl(cl::thread_pool *thread_pool,
                                  socket &&socket)
        : m_thread_pool(thread_pool)
        , m_socket(std::move(socket))
        , m_timer(m_thread_pool->get_io_context())
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

    write_future tcp_pipe_endpoint::impl::write_async(cl::buffer &&msg,
                                                      const std::optional<std::chrono::milliseconds> &timeout)
    {
        assert(msg.size() != 0);

        cl::ordered_lock guard(m_write_mtx, m_socket_mtx);
        if(!m_socket.is_open()) {
            return write_future(m_thread_pool, pipe_op_res::failed);
        }

        cl::promise promise(m_thread_pool, [](pipe_op_res r) { return r; });
        auto future = promise.get_future();

        const auto id = m_next_write_op_id++;
        std::optional<uint64_t> timer_id;
        if(timeout) {
            timer_id = m_timer.start([self = shared_from_this(), id]() {
                cl::ordered_lock guard(self->m_write_mtx, self->m_socket_mtx);
                auto it = std::find_if(self->m_write_operations.begin(),
                                       self->m_write_operations.end(),
                                       [id](const auto &v) { return v.id == id; });
                if(it != self->m_write_operations.end()) {
                    auto r = self->m_socket.is_open() ? pipe_op_res::timeout :
                            (self->m_was_invalidated_by_fail ? pipe_op_res::failed : pipe_op_res::canceled);
                    it->op.resolve_promise_once(r);
                    if(it->id != self->m_write_operations.back().id) {
                        self->m_write_operations.erase(it);
                    }
                }
            }, *timeout);
        }

        m_write_operations.emplace_front(std::move(msg), std::move(promise), id, timer_id);

        if(m_write_operations.size() == 1) {
            start_write_header_async();
        }
        return future;
    }

    read_future tcp_pipe_endpoint::impl::read_async(const std::optional<std::chrono::milliseconds> &timeout)
    {
        cl::ordered_lock guard(m_read_mtx, m_socket_mtx);
        if(!m_socket.is_open()) {
            return read_future(m_thread_pool, cl::ftuple(pipe_op_res::failed, cl::buffer{}));
        }

        cl::promise promise(m_thread_pool,
                            [](pipe_op_res r, cl::buffer &&b) {
                                return cl::ftuple(r, std::move(b));
                            });
        auto future = promise.get_future();

        const auto id = m_next_read_op_id++;
        std::optional<uint64_t> timer_id;
        if(timeout) {
            timer_id = m_timer.start([self = shared_from_this(), id]() {
                cl::ordered_lock guard(self->m_read_mtx, self->m_socket_mtx);
                auto it = std::find_if(self->m_read_operations.begin(),
                                       self->m_read_operations.end(),
                                       [id](const auto &v) { return v.id == id; });
                if(it != self->m_read_operations.end()) {
                    auto r = self->m_socket.is_open() ? pipe_op_res::timeout :
                            (self->m_was_invalidated_by_fail ? pipe_op_res::failed : pipe_op_res::canceled);
                    it->op.resolve_promise_once(r);
                    if(it->id != self->m_read_operations.back().id) {
                        self->m_read_operations.erase(it);
                    }
                }
            }, *timeout);
        }

        m_read_operations.emplace_front(std::move(promise), id, timer_id);

        if(m_read_operations.size() == 1) {
            start_read_header_async();
        }
        return future;
    }

    void tcp_pipe_endpoint::impl::start_write_header_async()
    {
        assert(!m_write_operations.empty());

        if(!m_socket.is_open()) {
            auto r = m_was_invalidated_by_fail ? pipe_op_res::failed : pipe_op_res::canceled;
            complete_all_operations_unsafe(r, m_write_operations);
            return;
        }

        m_socket.async_write_some(m_write_operations.back().op.get_unwritten_header_buffer(),
                                  [self = shared_from_this()](const boost::system::error_code &ec,
                                                              std::size_t bytes_transferred) mutable {
                                      self->on_header_operation(ec,
                                                                bytes_transferred,
                                                                self->m_write_operations,
                                                                self->m_write_mtx,
                                                                &tcp_pipe_endpoint::impl::start_write_header_async,
                                                                &tcp_pipe_endpoint::impl::start_write_payload_async);
                                  });
    }

    void tcp_pipe_endpoint::impl::start_write_payload_async()
    {
        assert(!m_write_operations.empty());

        if(!m_socket.is_open()) {
            auto r = m_was_invalidated_by_fail ? pipe_op_res::failed : pipe_op_res::canceled;
            complete_all_operations_unsafe(r, m_write_operations);
            return;
        }

        m_socket.async_write_some(m_write_operations.back().op.get_unwritten_payload_buffer(),
                                  [self = shared_from_this()](const boost::system::error_code &ec,
                                                              std::size_t bytes_transferred) mutable {
                                      self->on_payload_operation(ec,
                                                                 bytes_transferred,
                                                                 self->m_write_operations,
                                                                 self->m_write_mtx,
                                                                 &tcp_pipe_endpoint::impl::start_write_header_async,
                                                                 &tcp_pipe_endpoint::impl::start_write_payload_async);
                                  });
    }

    void tcp_pipe_endpoint::impl::start_read_header_async()
    {
        assert(!m_read_operations.empty());

        if(!m_socket.is_open()) {
            auto r = m_was_invalidated_by_fail ? pipe_op_res::failed : pipe_op_res::canceled;
            complete_all_operations_unsafe(r, m_read_operations);
            return;
        }

        m_socket.async_read_some(m_read_operations.back().op.get_buffer_for_unread_header(),
                                 [self = shared_from_this()](const boost::system::error_code &ec,
                                                             std::size_t bytes_transferred) mutable {
                                     self->on_header_operation(ec,
                                                                bytes_transferred,
                                                                self->m_read_operations,
                                                                self->m_read_mtx,
                                                                &tcp_pipe_endpoint::impl::start_read_header_async,
                                                                &tcp_pipe_endpoint::impl::start_read_payload_async);
                                 });
    }

    void tcp_pipe_endpoint::impl::start_read_payload_async()
    {
        assert(!m_read_operations.empty());

        if(!m_read_operations.back().op.is_payload_buffer_valid()) {
            invalidate_unsafe(false);
            complete_all_operations_unsafe(pipe_op_res::failed, m_read_operations);
            return;
        }

        if(!m_socket.is_open()) {
            auto r = m_was_invalidated_by_fail ? pipe_op_res::failed : pipe_op_res::canceled;
            complete_all_operations_unsafe(r, m_read_operations);
            return;
        }

        m_socket.async_read_some(m_read_operations.back().op.get_buffer_for_unread_payload(),
                                 [self = shared_from_this()](const boost::system::error_code &ec,
                                                             std::size_t bytes_transferred) mutable {
                                     self->on_payload_operation(ec,
                                                                bytes_transferred,
                                                                self->m_read_operations,
                                                                self->m_read_mtx,
                                                                &tcp_pipe_endpoint::impl::start_read_header_async,
                                                                &tcp_pipe_endpoint::impl::start_read_payload_async);
                                 });
    }

    template<typename Operations, typename OrderedMutex, typename StartHeaderOpAsync, typename StartPayloadOpAsync>
    void tcp_pipe_endpoint::impl::on_header_operation(const boost::system::error_code &ec,
                                                      std::size_t bytes,
                                                      Operations &operations,
                                                      OrderedMutex &op_mutex,
                                                      StartHeaderOpAsync start_header_op_async,
                                                      StartPayloadOpAsync start_payload_op_async)
    {
        cl::ordered_lock guard1(op_mutex, m_socket_mtx);

        assert(!operations.empty());

        auto &op = operations.back().op;
        op.add_bytes(bytes);

        if(ec) {
            if(ec == boost::asio::error::operation_aborted) {
                auto r = m_was_invalidated_by_fail ? pipe_op_res::failed : pipe_op_res::canceled;
                complete_all_operations_unsafe(r, operations);
            } else {
                invalidate_unsafe(false);

                complete_all_operations_unsafe(pipe_op_res::failed, operations);
            }
        } else {
            if(!op.is_header_completed()) {
                (this->*start_header_op_async)();
            } else {
                (this->*start_payload_op_async)();
            }
        }
    }

    template<typename Operations, typename OrderedMutex, typename StartHeaderOpAsync, typename StartPayloadOpAsync>
    void tcp_pipe_endpoint::impl::on_payload_operation(const boost::system::error_code &ec,
                                                       std::size_t bytes,
                                                       Operations &operations,
                                                       OrderedMutex &op_mutex,
                                                       StartHeaderOpAsync start_header_op_async,
                                                       StartPayloadOpAsync start_payload_op_async)
    {
        cl::ordered_lock guard1(op_mutex, m_socket_mtx);
        assert(!operations.empty());
        auto &op_data = operations.back();
        auto &op = op_data.op;
        op.add_bytes(bytes);

        if(ec) {
            if(ec == boost::asio::error::operation_aborted) {
                auto r = m_was_invalidated_by_fail ? pipe_op_res::failed : pipe_op_res::canceled;
                complete_all_operations_unsafe(r, operations);
            } else {
                invalidate_unsafe(false);
                complete_all_operations_unsafe(pipe_op_res::failed, operations);
            }
        } else {
            if(!op.is_payload_completed()) {
                (this->*start_payload_op_async)();
            } else {
                op.resolve_promise_once(pipe_op_res::success);

                if(op_data.timer_id) m_timer.cancel(*op_data.timer_id);
                operations.pop_back();

                if(!operations.empty()) {
                    (this->*start_header_op_async)();
                }
            }
        }
    }

    template<typename Operations>
    void tcp_pipe_endpoint::impl::complete_all_operations_unsafe(pipe_op_res r, Operations &operations)
    {
        while(!operations.empty()) {
            if(operations.back().timer_id) {
                m_timer.cancel(*operations.back().timer_id);
            }
            operations.back().op.resolve_promise_once(r);
            operations.pop_back();
        }
    }

    write_future tcp_pipe_endpoint::impl::write_async(cl::buffer &&msg)
    {
        return write_async(std::move(msg), std::nullopt);
    }

    read_future tcp_pipe_endpoint::impl::read_async()
    {
        return read_async(std::nullopt);
    }

    write_future tcp_pipe_endpoint::impl::write_async(cl::buffer &&msg, std::chrono::milliseconds timeout)
    {
        return write_async(std::move(msg), std::optional(timeout));
    }

    read_future tcp_pipe_endpoint::impl::read_async(std::chrono::milliseconds timeout)
    {
        return read_async(std::optional(timeout));
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

    tcp_pipe_endpoint::tcp_pipe_endpoint(cl::thread_pool *thread_pool,
                                         socket &&socket)
        : m_impl(std::make_shared<impl>(thread_pool, std::move(socket)))
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
