#ifdef _WIN32
#include "win-pipe-endpoint.h"

#include <list>
#include <condition_variable>

namespace vshalygin::rpc {
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

    std::shared_ptr<ipipe_endpoint> win_pipe_endpoint::create(win::pipe_handle &&handle,
                                                              std::shared_ptr<internal::win_pipe_iocp_owner> iocp_owner,
                                                              std::shared_ptr<cl::thread_pool> thread_pool)
    {
        return std::shared_ptr<ipipe_endpoint>(
            new win_pipe_endpoint(std::move(handle), std::move(iocp_owner), std::move(thread_pool)));
    }

    win_pipe_endpoint::win_pipe_endpoint(win::pipe_handle &&pipe,
                                         std::shared_ptr<internal::win_pipe_iocp_owner> iocp_owner,
                                         std::shared_ptr<cl::thread_pool> thread_pool)
        : m_pipe(std::move(pipe))
        , m_iocp_owner(std::move(iocp_owner))
        , m_thread_pool(std::move(thread_pool))
    {
        assert(m_pipe);
    }

    bool win_pipe_endpoint::is_connected() const
    {
        std::lock_guard g(m_pipe_mtx);
        return !m_pipe.empty();
    }

    void win_pipe_endpoint::set_disconnect_callback(cl::thread_pool_task<void()> &&callback)
    {
        std::lock_guard g(m_pipe_mtx);
        if(m_pipe) {
            m_disconnect_callbacks.push_back(std::move(callback));
        } else {
            callback.exec();
        }
    }

    win_pipe_endpoint::write_future win_pipe_endpoint::write_async(cl::buffer &&msg)
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

    win_pipe_endpoint::read_future win_pipe_endpoint::read_async()
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

    win_pipe_endpoint::write_future win_pipe_endpoint::write_async(
        cl::buffer && /*msg*/, std::chrono::milliseconds /*timeout*/)
    {
        //TODO add definition
        return {};
    }

    win_pipe_endpoint::read_future win_pipe_endpoint::read_async(
        std::chrono::milliseconds /*timeout*/)
    {
        //TODO add definition
        return {};
    }

    void win_pipe_endpoint::invalidate()
    {
        invalidate_impl(true);
    }

    void win_pipe_endpoint::complete_write_op()
    {
        std::unique_lock g(m_write_op_mtx);
        m_write_ops.pop_front();
        if(!m_write_ops.empty()) {
            m_iocp_owner->write_async(&m_write_ops.front());
        }
    }

    void win_pipe_endpoint::complete_read_op()
    {
        std::unique_lock g(m_read_op_mtx);
        m_read_ops.pop_front();
        if(!m_read_ops.empty()) {
            m_iocp_owner->read_async(&m_read_ops.front());
        }
    }

    void win_pipe_endpoint::invoke_disconnect_callbacks()
    {
        for(auto &cb : m_disconnect_callbacks) {
            cb.exec();
        }
        m_disconnect_callbacks.clear();
    }

    void win_pipe_endpoint::invalidate_impl(bool by_cancel)
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
}
#endif
