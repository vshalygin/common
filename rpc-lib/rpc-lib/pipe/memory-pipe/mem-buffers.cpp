#include "mem-buffers.h"

namespace vshalygin::rpc {
    mem_buffers::mem_buffers(std::shared_ptr<cl::thread_pool> thread_pool)
        : m_thread_pool(thread_pool)
        , m_client_to_server(mem_buffer::create(thread_pool))
        , m_server_to_client(mem_buffer::create(thread_pool))
    {}

    mem_buffers::~mem_buffers()
    {
        invalidate_impl(true, true);
    }

    mem_buffers::read_future
        mem_buffers::read_async_from_server(const std::optional<std::chrono::milliseconds> &timeout)
    {
        return m_server_to_client->read_async(timeout);
    }

    mem_buffers::read_future
        mem_buffers::read_async_from_client(const std::optional<std::chrono::milliseconds> &timeout)
    {
        return m_client_to_server->read_async(timeout);
    }

    mem_buffers::write_future
        mem_buffers::write_async_to_client(cl::buffer &&msg,
                                           const std::optional<std::chrono::milliseconds> &timeout)
    {
        return m_server_to_client->write_async(std::move(msg), timeout);
    }

    mem_buffers::write_future
        mem_buffers::write_async_to_server(cl::buffer &&msg,
                                           const std::optional<std::chrono::milliseconds> &timeout)
    {
        return m_client_to_server->write_async(std::move(msg), timeout);
    }

    void mem_buffers::set_invalidate_callback(cl::thread_pool_task<void()> &&callback)
    {
        std::lock_guard guard(m_mtx);
        if(m_invalidated) {
            callback.exec();
        } else {
            m_on_invalidate.push_back(std::move(callback));
        }
    }

    void mem_buffers::invalidate(bool by_server)
    {
        invalidate_impl(by_server, !by_server);
    }

    void mem_buffers::invalidate_impl(bool cancel_server_side, bool cancel_client_side)
    {
        m_client_to_server->invalidate(cancel_server_side);
        m_server_to_client->invalidate(cancel_client_side);

        std::lock_guard guard(m_mtx);
        m_invalidated = true;
        for(auto &cb : m_on_invalidate) {
            cb.exec();
        }
        m_on_invalidate.clear();
    }

    bool mem_buffers::is_valid() const
    {
        std::lock_guard guard(m_mtx);
        return !m_invalidated;
    }

    size_t mem_buffers::get_invalidate_callbacks_count() const
    {
        std::lock_guard guard(m_mtx);
        return m_on_invalidate.size();
    }
}
