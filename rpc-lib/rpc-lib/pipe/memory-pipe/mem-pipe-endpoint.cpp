#include "mem-pipe-endpoint.h"
#include "mem-buffers.h"

namespace vshalygin::rpc {
    mem_pipe_endpoint::mem_pipe_endpoint(bool is_server,
                                         std::shared_ptr<mem_buffers> mem_buffers)
        : m_is_server(is_server)
        , m_mem_buffers(std::move(mem_buffers))
    {}

    mem_pipe_endpoint::~mem_pipe_endpoint()
    {
        invalidate();
    }

    bool mem_pipe_endpoint::is_connected() const
    {
        return m_mem_buffers->is_valid();
    }

    void mem_pipe_endpoint::set_disconnect_callback(cl::thread_pool_task<void()> &&callback)
    {
        m_mem_buffers->set_invalidate_callback(std::move(callback));
    }

    mem_pipe_endpoint::write_future mem_pipe_endpoint::write_async(cl::buffer &&msg)
    {
        return write_async(std::move(msg), std::nullopt);
    }

    mem_pipe_endpoint::read_future mem_pipe_endpoint::read_async()
    {
        return read_async(std::nullopt);
    }

    mem_pipe_endpoint::write_future mem_pipe_endpoint::write_async(cl::buffer &&msg,
                                                                   std::chrono::milliseconds timeout)
    {
        return write_async(std::move(msg), std::optional(timeout));
    }

    mem_pipe_endpoint::read_future mem_pipe_endpoint::read_async(std::chrono::milliseconds timeout)
    {
        return read_async(std::optional(timeout));
    }

    mem_pipe_endpoint::write_future
        mem_pipe_endpoint::write_async(cl::buffer &&msg,
                                       const std::optional<std::chrono::milliseconds> &timeout)
    {
        return m_is_server ? m_mem_buffers->write_async_to_client(std::move(msg), timeout)
                           : m_mem_buffers->write_async_to_server(std::move(msg), timeout);
    }

    mem_pipe_endpoint::read_future
        mem_pipe_endpoint::read_async(const std::optional<std::chrono::milliseconds> &timeout)
    {
        return m_is_server ? m_mem_buffers->read_async_from_client(timeout)
                           : m_mem_buffers->read_async_from_server(timeout);
    }

    void mem_pipe_endpoint::invalidate()
    {
        m_mem_buffers->invalidate();
    }
}
