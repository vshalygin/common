#include "mem-buffers.h"

namespace vshalygin::rpc {
    mem_buffers::mem_buffers(std::shared_ptr<cl::thread_pool> thread_pool)
        : m_thread_pool(thread_pool)
        , m_client_to_server(mem_buffer::create(thread_pool))
        , m_server_to_client(mem_buffer::create(thread_pool))
    {}

    mem_buffers::~mem_buffers()
    {
        invalidate();
    }

    mem_buffers::read_future mem_buffers::read_async_from_server()
    {
        return m_server_to_client->read_async();
    }

    mem_buffers::read_future mem_buffers::read_async_from_client()
    {
        return m_client_to_server->read_async();
    }

    mem_buffers::write_future mem_buffers::write_async_to_client(cl::buffer &&msg)
    {
        return m_server_to_client->write_async(std::move(msg));
    }

    mem_buffers::write_future mem_buffers::write_async_to_server(cl::buffer &&msg)
    {
        return m_client_to_server->write_async(std::move(msg));
    }

    void mem_buffers::set_invalidate_callback(cl::thread_pool_task<void()> &&callback)
    {
        std::lock_guard guard(m_mtx);
        if(m_invalidated) {
            m_thread_pool->post(std::move(callback));
        } else {
            m_on_invalidate.push_back(std::move(callback));
        }
    }

    void mem_buffers::invalidate()
    {
        m_client_to_server->invalidate();
        m_server_to_client->invalidate();

        std::lock_guard guard(m_mtx);
        m_invalidated = true;
        for(auto &cb : m_on_invalidate) {
            m_thread_pool->post(std::move(cb));
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
