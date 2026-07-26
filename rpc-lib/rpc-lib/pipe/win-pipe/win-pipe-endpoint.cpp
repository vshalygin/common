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
        void wait_current_write_ops_completed();
        void wait_current_read_ops_completed();

        void complete_write_op();
        void complete_read_op();

        void set_disconnect_on_op_failed();

        void cancel();

    private:
        mutable std::mutex m_pipe_mtx;
        bool m_is_connected = true;
        cl::thread_pool_task<void()> m_disconnect_callback;
        win::pipe_handle m_pipe;

        std::shared_ptr<internal::win_pipe_iocp_owner> m_iocp_owner;
        std::shared_ptr<cl::thread_pool> m_thread_pool;

        mutable std::mutex m_write_op_mtx;
        std::condition_variable m_write_op_cv;
        std::list<write_op> m_write_ops;

        mutable std::mutex m_read_op_mtx;
        std::condition_variable m_read_op_cv;
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
        return m_is_connected;
    }

    void win_pipe_endpoint::impl::set_disconnect_callback(cl::thread_pool_task<void()> &&callback)
    {
        std::lock_guard g(m_pipe_mtx);
        if(m_is_connected) {
            m_disconnect_callback = std::move(callback);
        } else {
            callback.exec();
        }
    }

    win_pipe_endpoint::write_future win_pipe_endpoint::impl::write_async(cl::buffer &&msg)
    {
        std::lock_guard g(m_pipe_mtx);
        if(!m_is_connected) {
            return write_future(m_thread_pool.get(), pipe_op_res::failed);
        }

        std::lock_guard gg(m_write_op_mtx);
        auto &op = m_write_ops.emplace_front(m_pipe.get(), std::move(msg), m_thread_pool.get());
        if(m_write_ops.size() == 1) {
            m_iocp_owner->write_async(&m_write_ops.front());
        }

        return op.get_future()
            .then([self = weak_from_this()](op_res r) {
                      if(auto s = self.lock()) {
                          if(r == op_res::failed) {
                              s->set_disconnect_on_op_failed();
                          }
                          
                          s->complete_write_op();
                      }

                      return to_pipe_op_res(r);
                  });

    }

    win_pipe_endpoint::read_future win_pipe_endpoint::impl::read_async()
    {
        std::lock_guard g(m_pipe_mtx);
        if(!m_is_connected) {
            return read_future(m_thread_pool.get(), ftuple(pipe_op_res::failed, cl::buffer{}));
        }

        std::lock_guard gg(m_read_op_mtx);
        auto &op = m_read_ops.emplace_front(m_pipe.get(), m_thread_pool.get());
        if(m_read_ops.size() == 1) {
            m_iocp_owner->read_async(&m_read_ops.front());
        }

        return op.get_future()
            .then([self = weak_from_this()](op_res r, cl::buffer &&b) {
                      if(auto s = self.lock()) {
                          if(r == op_res::failed) {
                              s->set_disconnect_on_op_failed();
                          }

                          s->complete_read_op();
                      }
                      
                      return ftuple(to_pipe_op_res(r), std::move(b));
                  });
    }

    win_pipe_endpoint::write_future win_pipe_endpoint::impl::write_async(
        cl::buffer && /*msg*/, std::chrono::milliseconds /*timeout*/)
    {
        //TODO add definition
        return {};
    }

    win_pipe_endpoint::read_future win_pipe_endpoint::impl::read_async(
        std::chrono::milliseconds /*timeout*/)
    {
        //TODO add definition
        return {};
    }

    void win_pipe_endpoint::impl::invalidate()
    {
        std::unique_lock pipe_lock(m_pipe_mtx);
        if(!m_pipe) {
            return;
        }

        if(m_is_connected) {
            m_is_connected = false;

            std::unique_lock write_lock(m_write_op_mtx);
            if(!m_write_ops.empty()) {
                auto it = m_write_ops.begin();
                it->set_canceled_if_possible();
                m_iocp_owner->cancel_write(&(*it));
                for(++it; it != m_write_ops.end(); ++it) {
                    it->set_canceled_if_possible();
                }
            }
            write_lock.unlock();

            std::unique_lock read_lock(m_read_op_mtx);
            if(!m_read_ops.empty()) {
                auto it = m_read_ops.begin();
                it->set_canceled_if_possible();
                m_iocp_owner->cancel_read(&(*it));
                for(++it; it != m_read_ops.end(); ++it) {
                    it->set_canceled_if_possible();
                }
            }
            read_lock.unlock();

            if(m_disconnect_callback) {
                m_disconnect_callback.exec();
                m_disconnect_callback = {};
            }
        }

        pipe_lock.unlock();

        wait_current_write_ops_completed();
        wait_current_read_ops_completed();

        pipe_lock.lock();
        m_pipe.reset();
    }

    void win_pipe_endpoint::impl::wait_current_write_ops_completed()
    {
        std::unique_lock g(m_write_op_mtx);
        m_write_op_cv.wait(g, [this]() { return m_write_ops.empty(); });
    }

    void win_pipe_endpoint::impl::wait_current_read_ops_completed()
    {
        std::unique_lock g(m_read_op_mtx);
        m_read_op_cv.wait(g, [this]() { return m_read_ops.empty(); });
    }

    void win_pipe_endpoint::impl::complete_write_op()
    {
        bool notify = false;

        {
            std::unique_lock g(m_write_op_mtx);
            m_write_ops.pop_front();
            auto size = m_write_ops.size();
            if(size == 0) {
                notify = true;
            } else {
                m_iocp_owner->write_async(&m_write_ops.front());
            }
        }

        if(notify) {
            m_write_op_cv.notify_all();
        }
    }

    void win_pipe_endpoint::impl::complete_read_op()
    {
        bool notify = false;

        {
            std::unique_lock g(m_read_op_mtx);
            m_read_ops.pop_front();
            auto size = m_read_ops.size();
            if(size == 0) {
                notify = true;
            } else {
                m_iocp_owner->read_async(&m_read_ops.front());
            }
        }

        if(notify) {
            m_read_op_cv.notify_all();
        }
    }

    void win_pipe_endpoint::impl::set_disconnect_on_op_failed()
    {
        std::lock_guard g(m_pipe_mtx);
        std::lock_guard gg(m_write_op_mtx);
        std::lock_guard ggg(m_read_op_mtx);

        m_is_connected = false;
        for(auto it = m_write_ops.begin(); it != m_write_ops.end(); ++it) {
            it->set_failed_if_possible();
        }
        for(auto it = m_read_ops.begin(); it != m_read_ops.end(); ++it) {
            it->set_failed_if_possible();
        }

        if(m_disconnect_callback) {
            m_disconnect_callback.exec();
            m_disconnect_callback = {};
        }
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
