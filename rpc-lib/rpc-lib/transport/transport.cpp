#include "transport.h"
#include "rpc-lib/pipe/ipipe-endpoint.h"
#include "common-lib/thread/thread-pool/thread-pool.h"
#include <cassert>

namespace vshalygin::rpc {
    transport::transport(std::shared_ptr<cl::thread_pool> thread_pool,
                         std::shared_ptr<ipipe_endpoint> pipe_endpoint,
                         cl::thread_pool_task<void()> &&start_callback,
                         cl::thread_pool_task<void()> &&stop_callback)
        : m_pipe_endpoint(std::move(pipe_endpoint))
    {
        assert(m_pipe_endpoint && m_pipe_endpoint->is_connected());

        thread_pool->post(std::move(start_callback));
        m_pipe_endpoint->set_disconnect_callback(std::move(stop_callback));
    }

    transport::~transport()
    {
        stop();
    }

    transport::send_future transport::send_async(cl::buffer &&message)
    {
        return m_pipe_endpoint->write_async(std::move(message));
    }

    transport::recv_future transport::recv_async()
    {
        return m_pipe_endpoint->read_async();
    }

    void transport::stop()
    {
        if(!m_stopped_requested.exchange(true, std::memory_order_acq_rel)) {
            m_pipe_endpoint->invalidate();
        }
    }

    bool transport::is_running() const
    {
        return m_pipe_endpoint->is_connected();
    }
}
