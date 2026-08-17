#ifdef _WIN32
#include "win-pipe-endpoint.h"

#include <list>
#include <condition_variable>

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
                      std::shared_ptr<cl::thread_pool> thread_pool);

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
        void complete_write_op();
        void complete_read_op();

        void invoke_disconnect_callbacks();

        void invalidate_impl(bool by_cancel);

    private:
        mutable std::mutex m_pipe_mtx;
        std::vector<cl::thread_pool_task<void()>> m_disconnect_callbacks;
        win::pipe_handle m_pipe;

        std::shared_ptr<internal::win_pipe_iocp_owner> m_iocp_owner;
        std::shared_ptr<cl::thread_pool> m_thread_pool;

        mutable std::mutex m_write_op_mtx;
        std::list<write_op> m_write_ops;

        mutable std::mutex m_read_op_mtx;
        std::list<read_op> m_read_ops;
    };

    win_pipe_endpoint::impl::impl(win::pipe_handle &&pipe,
                                  std::shared_ptr<internal::win_pipe_iocp_owner> iocp_owner,
                                  std::shared_ptr<cl::thread_pool> thread_pool)
        : m_pipe(std::move(pipe))
        , m_iocp_owner(std::move(iocp_owner))
        , m_thread_pool(std::move(thread_pool))
    {
        assert(m_pipe);
    }

    bool win_pipe_endpoint::impl::is_connected() const
    {
        std::lock_guard g(m_pipe_mtx);
        return !m_pipe.empty();
    }

    void win_pipe_endpoint::impl::set_disconnect_callback(cl::thread_pool_task<void()> &&callback)
    {
        std::lock_guard g(m_pipe_mtx);
        if(m_pipe) {
            m_disconnect_callbacks.push_back(std::move(callback));
        } else {
            callback.exec();
        }
    }

    win_pipe_endpoint::write_future win_pipe_endpoint::impl::write_async(cl::buffer &&msg)
    {
        std::lock_guard g(m_pipe_mtx);
        if(!m_pipe) {
            return write_future(m_thread_pool.get(), pipe_op_res::failed);
        }

        std::lock_guard gg(m_write_op_mtx);
        auto &op = m_write_ops.emplace_front(m_pipe.get(), std::move(msg), m_thread_pool.get());
        if(m_write_ops.size() == 1) {
            m_iocp_owner->write_async(&m_write_ops.front());
        }

        return op.get_future()
            .then([self = shared_from_this()](op_res r) {
                      if(r == op_res::failed) {
                          self->invalidate_impl(false);
                      }

                      self->complete_write_op();

                      return to_pipe_op_res(r);
                  });
    }

    win_pipe_endpoint::read_future win_pipe_endpoint::impl::read_async()
    {
        std::lock_guard g(m_pipe_mtx);
        if(!m_pipe) {
            return read_future(m_thread_pool.get(), ftuple(pipe_op_res::failed, cl::buffer{}));
        }

        std::lock_guard gg(m_read_op_mtx);
        auto &op = m_read_ops.emplace_front(m_pipe.get(), m_thread_pool.get());
        if(m_read_ops.size() == 1) {
            m_iocp_owner->read_async(&m_read_ops.front());
        }

        return op.get_future()
            .then([self = shared_from_this()](op_res r, cl::buffer &&b) {
                      if(r == op_res::failed) {
                          self->invalidate_impl(false);
                      }

                      self->complete_read_op();
                      
                      return ftuple(to_pipe_op_res(r), std::move(b));
                  });
    }

    win_pipe_endpoint::write_future win_pipe_endpoint::impl::write_async(cl::buffer && /*msg*/,
                                                                         std::chrono::milliseconds /*timeout*/)
    {
        //TODO add definition
        return {};
    }

    win_pipe_endpoint::read_future win_pipe_endpoint::impl::read_async(std::chrono::milliseconds /*timeout*/)
    {
        //TODO add definition
        return {};
    }

    void win_pipe_endpoint::impl::invalidate()
    {
        invalidate_impl(true);
    }

    void win_pipe_endpoint::impl::complete_write_op()
    {
        std::unique_lock g(m_write_op_mtx);
        m_write_ops.pop_back();
        if(!m_write_ops.empty()) {
            m_iocp_owner->write_async(&m_write_ops.back());
        }
    }

    void win_pipe_endpoint::impl::complete_read_op()
    {
        std::unique_lock g(m_read_op_mtx);
        m_read_ops.pop_back();
        if(!m_read_ops.empty()) {
            m_iocp_owner->read_async(&m_read_ops.back());
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
        std::lock_guard g(m_pipe_mtx);
        if(!m_pipe) {
            return;
        }

        std::lock_guard gg(m_write_op_mtx);
        std::lock_guard ggg(m_read_op_mtx);

        auto set_write_result = by_cancel ? &write_op::set_canceled_if_possible
                                          : &write_op::set_failed_if_possible;
        auto set_read_result = by_cancel ? &read_op::set_canceled_if_possible
                                         : &read_op::set_failed_if_possible;

        for(auto it = m_write_ops.begin(); it != m_write_ops.end(); ++it) {
            ((*it).*set_write_result)();
        }
        for(auto it = m_read_ops.begin(); it != m_read_ops.end(); ++it) {
            ((*it).*set_read_result)();
        }

        m_pipe.reset();

        invoke_disconnect_callbacks();
    }

    win_pipe_endpoint::win_pipe_endpoint(win::pipe_handle &&handle,
                                         std::shared_ptr<internal::win_pipe_iocp_owner> iocp_owner,
                                         std::shared_ptr<cl::thread_pool> thread_pool)
        : m_impl(std::make_shared<impl>(std::move(handle), std::move(iocp_owner), std::move(thread_pool)))
    {}

    win_pipe_endpoint::~win_pipe_endpoint()
    {
        m_impl->invalidate();
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
