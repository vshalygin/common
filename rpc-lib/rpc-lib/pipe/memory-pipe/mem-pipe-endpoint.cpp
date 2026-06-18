#include "mem-pipe-endpoint.h"
#include "mem-buffers.h"
#include "common-lib/syncronization/event/event.h"

#include <mutex>
#include <condition_variable>

using microseconds = std::chrono::microseconds;

namespace vshalygin::rpc {
    mem_pipe_endpoint::mem_pipe_endpoint(bool is_server,
                                         std::shared_ptr<cl::thread_pool> thread_pool)
        : m_is_server(is_server)
        , m_thread_pool(thread_pool)
    {}

    mem_pipe_endpoint::~mem_pipe_endpoint()
    {
        invalidate();
    }

    void mem_pipe_endpoint::set_buffers(std::shared_ptr<mem_buffers> mem_buffers)
    {
        {
            std::lock_guard guard(m_mtx);
            m_mem_buffers = std::move(mem_buffers);
            if(m_is_invalidated) {
                m_mem_buffers->invalidate();
            }

            if(m_on_disconnect) {
                m_mem_buffers->set_invalidate_callback(std::move(m_on_disconnect));
                m_on_disconnect = {};
            }
        }

        m_cv.notify_all();
    }

    bool mem_pipe_endpoint::is_connected() const
    {
        std::lock_guard guard(m_mtx);
        return m_mem_buffers && m_mem_buffers->is_valid();
    }

    void mem_pipe_endpoint::subscribe_to_disconnect(std::function<void()> &&callback)
    {
        std::lock_guard guard(m_mtx);
        assert(!m_on_disconnect);
        if(m_mem_buffers) {
            m_mem_buffers->set_invalidate_callback(std::move(callback));
        } else {
            m_on_disconnect = std::move(callback);
        }
    }

    pipe_wait_res mem_pipe_endpoint::wait_connect_for(const std::chrono::microseconds &mcs) const
    {
        auto now = std::chrono::steady_clock::now();
        std::unique_lock lock(m_mtx);
        auto r = m_cv.wait_until(lock, now + mcs,
                                [this]() {
                                    return m_is_invalidated || m_mem_buffers != nullptr;
                                });

        return r ? (m_is_invalidated ? pipe_wait_res::invalidated : pipe_wait_res::connected)
                 : pipe_wait_res::timeout;
    }

    pipe_wait_res mem_pipe_endpoint::wait_connect() const
    {
        std::unique_lock lock(m_mtx);
        m_cv.wait(lock,[this]() {
                           return m_is_invalidated || m_mem_buffers != nullptr;
                       });

        return m_is_invalidated ? pipe_wait_res::invalidated : pipe_wait_res::connected;
    }

    void mem_pipe_endpoint::write_async(cl::buffer &&msg,
                                        std::function<void(pipe_op_res)> &&callback)
    {
        std::lock_guard guard(m_mtx);
        if(!m_mem_buffers) {
            m_thread_pool->post([c = std::move(callback)]() { c(pipe_op_res::failed); });
            return;
        }

        if(m_is_server) {
            m_mem_buffers->write_async_to_client(std::move(msg), std::move(callback));
        } else {
            m_mem_buffers->write_async_to_server(std::move(msg), std::move(callback));
        }
    }

    void mem_pipe_endpoint::read_async(read_callback_t &&callback)
    {
        std::lock_guard guard(m_mtx);
        if(!m_mem_buffers) {
            m_thread_pool->post([c = std::move(callback)]() { c(pipe_op_res::failed, {}); });
            return;
        }

        if(m_is_server) {
            m_mem_buffers->read_async_from_client(std::move(callback));
        } else {
            m_mem_buffers->read_async_from_server(std::move(callback));
        }
    }

    bool mem_pipe_endpoint::try_to_write_for(cl::buffer &&msg, const microseconds &timeout)
    {
        struct data
        {
            cl::event sync_event;
            pipe_op_res res = pipe_op_res::failed;
        };
        auto d = std::make_shared<data>();
        auto task = [d](pipe_op_res r) {
            d->res = r;
            d->sync_event.set();
        };

        write_async(std::move(msg), std::move(task));

        if(!d->sync_event.wait_for(timeout)) {
            return false;
        }

        return is_success(d->res);
    }

    std::optional<cl::buffer> mem_pipe_endpoint::try_to_read_for(const microseconds &timeout)
    {
        struct data
        {
            cl::event sync_event;
            pipe_op_res res = pipe_op_res::failed;
            cl::buffer buf;
        };
        auto d = std::make_shared<data>();
        auto task = [d](pipe_op_res r, cl::buffer &&b) {
            d->res = r;
            d->buf = std::move(b);
            d->sync_event.set();
        };

        read_async(std::move(task));

        if(!d->sync_event.wait_for(timeout)) {
            return std::nullopt;
        }

        return is_success(d->res) ? std::optional<cl::buffer>(std::move(d->buf)) : std::nullopt;
    }

    void mem_pipe_endpoint::invalidate()
    {
        {
            std::lock_guard lock(m_mtx);
            m_is_invalidated = true;

            if(m_mem_buffers) {
                m_mem_buffers->invalidate();
            }
        }

        m_cv.notify_all();
    }
}
