#include "mem-pipe-endpoint.h"
#include "mem-buffers.h"
#include "common-lib/synchronization/event/event.h"

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

    void mem_pipe_endpoint::set_disconnect_callback(cl::thread_pool_task<void()> &&callback)
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

    mem_pipe_endpoint::write_future mem_pipe_endpoint::write_async(cl::buffer &&msg)
    {
        std::lock_guard guard(m_mtx);
        if(!m_mem_buffers) {
            auto promise = make_promise(m_thread_pool.get(), []() {
                return pipe_op_res::failed;
            });
            promise.resolve();
            return promise.get_future();
        }
        
        return m_is_server ? m_mem_buffers->write_async_to_client(std::move(msg))
                           : m_mem_buffers->write_async_to_server(std::move(msg));
    }

    mem_pipe_endpoint::read_future mem_pipe_endpoint::read_async()
    {
        std::lock_guard guard(m_mtx);
        if(!m_mem_buffers) {
            auto promise = make_promise(m_thread_pool.get(), []() {
                return ftuple(pipe_op_res::failed, cl::buffer{});
            });
            promise.resolve();
            return promise.get_future();
        }
        
        return m_is_server ? m_mem_buffers->read_async_from_client()
                           : m_mem_buffers->read_async_from_server();
    }

    void mem_pipe_endpoint::invalidate()
    {
        {
            std::lock_guard lock(m_mtx);
            if(m_is_invalidated) {
                return;
            }

            m_is_invalidated = true;

            if(m_mem_buffers) {
                m_mem_buffers->invalidate();
            } else if(m_on_disconnect) {
                m_thread_pool->post(std::move(m_on_disconnect));
                m_on_disconnect = {};
            }
        }

        m_cv.notify_all();
    }
}
