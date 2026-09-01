#ifdef _WIN32
#include "win-pipe-endpoint.h"

#include <common-lib/synchronization/value-locker.h>
#include <common-lib/timer/multiple-timer.h>

#include <list>
#include <optional>

namespace vshalygin::rpc {
    using write_op = internal::win_pipe_write_operation;
    using read_op = internal::win_pipe_read_operation;
    using op_res = internal::win_pipe_operation_res;

    namespace {
        pipe_op_res to_pipe_op_res(op_res r)
        {
            switch(r) {
                case op_res::canceled:
                    return pipe_op_res::canceled;
                case op_res::success:
                    return pipe_op_res::success;
                case op_res::timeout:
                    return pipe_op_res::timeout;
                case op_res::failed:
                    return pipe_op_res::failed;
                default:
                    assert(!"unknown win pipe op res");
                    return pipe_op_res::failed;
            }
        }
    }

    class win_pipe_endpoint::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        explicit impl(win::pipe_handle &&pipe,
                      std::shared_ptr<internal::win_pipe_iocp_owner> iocp_owner,
                      cl::thread_pool *thread_pool);

        impl(const impl &) = delete;
        impl &operator=(const impl &) = delete;

        bool is_connected() const;
        void set_disconnect_callback(cl::thread_pool_task<void()> &&callback);

        write_future write_async(cl::buffer &&msg);
        read_future read_async();
        write_future write_async(cl::buffer &&msg, std::chrono::milliseconds timeout);
        read_future read_async(std::chrono::milliseconds timeout);

        void invalidate();

    private:
        write_future write_async(cl::buffer &&msg, std::optional<std::chrono::milliseconds> timeout);
        read_future read_async(std::optional<std::chrono::milliseconds> timeout);

        void complete_write_op(uint64_t id, std::optional<uint64_t> timer_id);
        void complete_read_op(uint64_t id, std::optional<uint64_t> timer_id);

        void cancel_write_op_by_timeout(uint64_t op_id);
        void cancel_read_op_by_timeout(uint64_t op_id);

        void invoke_disconnect_callbacks();

        void invalidate_impl(bool by_cancel);

    private:
        std::vector<cl::thread_pool_task<void()>> m_disconnect_callbacks;
        std::shared_ptr<cl::value_locker<win::pipe_handle>> m_pipe;

        std::shared_ptr<internal::win_pipe_iocp_owner> m_iocp_owner;
        cl::thread_pool *m_thread_pool;

        mutable std::mutex m_write_op_mtx;
        struct write_op_data
        {
            uint64_t id;
            std::shared_ptr<write_op> op;
            std::optional<uint64_t> timer_id;
        };
        uint64_t m_next_write_op_id = 0;
        std::list<write_op_data> m_write_ops;

        mutable std::mutex m_read_op_mtx;
        struct read_op_data
        {
            uint64_t id;
            std::shared_ptr<read_op> op;
            std::optional<uint64_t> timer_id;
        };
        uint64_t m_next_read_op_id = 0;
        std::list<read_op_data> m_read_ops;

        cl::multiple_timer m_timer;
    };

    win_pipe_endpoint::impl::impl(win::pipe_handle &&pipe,
                                  std::shared_ptr<internal::win_pipe_iocp_owner> iocp_owner,
                                  cl::thread_pool *thread_pool)
        : m_pipe(std::make_shared<cl::value_locker<win::pipe_handle>>(std::move(pipe)))
        , m_iocp_owner(std::move(iocp_owner))
        , m_thread_pool(thread_pool)
        , m_timer(m_thread_pool->get_io_context())
    {
        assert(is_connected());
    }

    bool win_pipe_endpoint::impl::is_connected() const
    {
        return !m_pipe->lock()->empty();
    }

    void win_pipe_endpoint::impl::set_disconnect_callback(cl::thread_pool_task<void()> &&callback)
    {
        auto pipe = m_pipe->lock();
        if(!pipe->empty()) {
            m_disconnect_callbacks.push_back(std::move(callback));
        } else {
            callback.exec();
        }
    }

    win_pipe_endpoint::read_future win_pipe_endpoint::impl::read_async()
    {
        return read_async(std::nullopt);
    }

    win_pipe_endpoint::read_future win_pipe_endpoint::impl::read_async(std::chrono::milliseconds timeout)
    {
        return read_async(std::optional(timeout));
    }

    win_pipe_endpoint::write_future win_pipe_endpoint::impl::write_async(cl::buffer &&msg)
    {
        return write_async(std::move(msg), std::nullopt);
    }

    win_pipe_endpoint::write_future win_pipe_endpoint::impl::write_async(cl::buffer &&msg,
                                                                         std::chrono::milliseconds timeout)
    {
        return write_async(std::move(msg), std::optional(timeout));
    }

    win_pipe_endpoint::write_future win_pipe_endpoint::impl::write_async(
        cl::buffer &&msg, std::optional<std::chrono::milliseconds> timeout)
    {
        auto pipe = m_pipe->lock();
        if(pipe->empty()) {
            return write_future(m_thread_pool, pipe_op_res::failed);
        }

        std::lock_guard gg(m_write_op_mtx);
        auto id = m_next_write_op_id++;
        std::optional<uint64_t> timer_id;
        if(timeout) {
            timer_id = m_timer.start([s = shared_from_this(), id]() {
                s->cancel_write_op_by_timeout(id);
            }, *timeout);
        }

        auto &data = m_write_ops.emplace_front(write_op_data{
            id, write_op::create(m_pipe, std::move(msg), m_thread_pool), timer_id });
        if(m_write_ops.size() == 1) {
            m_iocp_owner->write_async(data.op);
        }

        return data.op->get_future()
            .then([self = shared_from_this(), timer_id, id](auto result) mutable {
                return result.lock().with([&](op_res r) {
                      if(r == op_res::failed) {
                          self->invalidate_impl(false);
                      }

                      self->complete_write_op(id, timer_id);

                      return to_pipe_op_res(r);
                  });
            });
    }

    win_pipe_endpoint::read_future win_pipe_endpoint::impl::read_async(
        std::optional<std::chrono::milliseconds> timeout)
    {
        auto pipe = m_pipe->lock();
        if(pipe->empty()) {
            return read_future(m_thread_pool, cl::ftuple(pipe_op_res::failed, cl::buffer{}));
        }

        std::lock_guard gg(m_read_op_mtx);
        auto id = m_next_read_op_id++;
        std::optional<uint64_t> timer_id;
        if(timeout) {
            timer_id = m_timer.start([s = shared_from_this(), id]() {
                s->cancel_read_op_by_timeout(id);
            }, *timeout);
        }
        auto &data = m_read_ops.emplace_front(read_op_data{
            id, read_op::create(m_pipe, m_thread_pool), timer_id });
        if(m_read_ops.size() == 1) {
            m_iocp_owner->read_async(m_read_ops.front().op);
        }

        return data.op->get_future()
            .then([self = shared_from_this(), timer_id, id](auto result) mutable {
                return result.lock().with([&](op_res r, cl::buffer &&b) {
                      if(r == op_res::failed) {
                          self->invalidate_impl(false);
                      }

                      self->complete_read_op(id, timer_id);

                      return cl::ftuple(to_pipe_op_res(r), std::move(b));
                  });
            });
    }

    void win_pipe_endpoint::impl::invalidate()
    {
        invalidate_impl(true);
    }

    void win_pipe_endpoint::impl::complete_write_op(uint64_t id, std::optional<uint64_t> timer_id)
    {
        std::unique_lock g(m_write_op_mtx);
        if(!m_write_ops.empty() && m_write_ops.back().id == id) {
            if(timer_id.has_value()) {
                m_timer.cancel(*timer_id);
            }

            m_write_ops.pop_back();
            if(!m_write_ops.empty()) {
                m_iocp_owner->write_async(m_write_ops.back().op);
            }
        }
    }

    void win_pipe_endpoint::impl::complete_read_op(uint64_t id, std::optional<uint64_t> timer_id)
    {
        std::unique_lock g(m_read_op_mtx);
        if(!m_read_ops.empty() && m_read_ops.back().id == id) {
            if(timer_id.has_value()) {
                m_timer.cancel(*timer_id);
            }

            m_read_ops.pop_back();
            if(!m_read_ops.empty()) {
                m_iocp_owner->read_async(m_read_ops.back().op);
            }
        }
    }

    void win_pipe_endpoint::impl::cancel_write_op_by_timeout(uint64_t op_id)
    {
        std::shared_ptr<write_op> operation_to_resolve;
        {
            std::unique_lock g(m_write_op_mtx);

            auto it = std::find_if(
                m_write_ops.begin(), m_write_ops.end(),
                [op_id](auto &v) { return v.id == op_id; });
            if(it != m_write_ops.end()) {
                it->op->set_timeout_if_possible();
                if(it->id == m_write_ops.back().id) {
                    m_iocp_owner->cancel_write(it->op);
                } else {
                    operation_to_resolve = std::move(it->op);
                    m_write_ops.erase(it);
                }
            }
        }

        if(operation_to_resolve) {
            operation_to_resolve->resolve();
        }
    }

    void win_pipe_endpoint::impl::cancel_read_op_by_timeout(uint64_t op_id)
    {
        std::shared_ptr<read_op> operation_to_resolve;
        {
            std::unique_lock g(m_read_op_mtx);

            auto it = std::find_if(
                m_read_ops.begin(), m_read_ops.end(),
                [op_id](auto &v) { return v.id == op_id; });
            if(it != m_read_ops.end()) {
                it->op->set_timeout_if_possible();
                if(it->id == m_read_ops.back().id) {
                    m_iocp_owner->cancel_read(it->op);
                } else {
                    operation_to_resolve = std::move(it->op);
                    m_read_ops.erase(it);
                }
            }
        }

        if(operation_to_resolve) {
            operation_to_resolve->resolve();
        }
    }

    void win_pipe_endpoint::impl::invoke_disconnect_callbacks()
    {
        for(auto &cb : m_disconnect_callbacks) {
            cb.exec();
        }
        m_disconnect_callbacks.clear();
    }

    void win_pipe_endpoint::impl::invalidate_impl(bool by_cancel)
    {
        auto pipe = m_pipe->lock();
        if(pipe->empty()) {
            return;
        }

        std::lock_guard gg(m_write_op_mtx);
        std::lock_guard ggg(m_read_op_mtx);

        auto set_write_result = by_cancel ? &write_op::set_canceled_if_possible
                                          : &write_op::set_failed_if_possible;
        auto set_read_result = by_cancel ? &read_op::set_canceled_if_possible
                                         : &read_op::set_failed_if_possible;

        for(auto it = m_write_ops.begin(); it != m_write_ops.end(); ++it) {
            (*(it->op).*set_write_result)();
        }
        for(auto it = m_read_ops.begin(); it != m_read_ops.end(); ++it) {
            (*(it->op).*set_read_result)();
        }

        pipe->reset();

        invoke_disconnect_callbacks();
    }

    win_pipe_endpoint::win_pipe_endpoint(win::pipe_handle &&handle,
                                         std::shared_ptr<internal::win_pipe_iocp_owner> iocp_owner,
                                         cl::thread_pool *thread_pool)
        : m_impl(std::make_shared<impl>(std::move(handle), std::move(iocp_owner), thread_pool))
    {}

    win_pipe_endpoint::~win_pipe_endpoint()
    {
        invalidate();
    }

    bool win_pipe_endpoint::is_connected() const
    {
        return m_impl->is_connected();
    }

    void win_pipe_endpoint::set_disconnect_callback(cl::thread_pool_task<void()> &&callback)
    {
        m_impl->set_disconnect_callback(std::move(callback));
    }

    win_pipe_endpoint::write_future win_pipe_endpoint::write_async(cl::buffer &&msg)
    {
        return m_impl->write_async(std::move(msg));
    }

    win_pipe_endpoint::read_future win_pipe_endpoint::read_async()
    {
        return m_impl->read_async();
    }

    win_pipe_endpoint::write_future win_pipe_endpoint::write_async(
        cl::buffer &&msg, std::chrono::milliseconds timeout)
    {
        return m_impl->write_async(std::move(msg), timeout);
    }

    win_pipe_endpoint::read_future win_pipe_endpoint::read_async(
        std::chrono::milliseconds timeout)
    {
        return m_impl->read_async(timeout);
    }

    void win_pipe_endpoint::invalidate()
    {
        return m_impl->invalidate();
    }
}
#endif
