#include "transport.h"
#include "rpc-lib/pipe/ipipe-endpoint.h"
#include "common-lib/thread-pool/thread-pool.h"
#include <cassert>

namespace vshalygin::rpc {
    transport::transport(std::shared_ptr<cl::thread_pool> thread_pool,
                         std::shared_ptr<ipipe_endpoint> pipe_endpoint,
                         std::function<void()> &&stop_callback)
        : m_thread_pool(std::move(thread_pool))
        , m_pipe_endpoint(std::move(pipe_endpoint))
    {
        assert(m_thread_pool);
        assert(m_pipe_endpoint && m_pipe_endpoint->is_connected());

        m_pipe_endpoint->subscribe_to_disconnect(std::move(stop_callback));
    }

    transport::~transport()
    {
        stop();
    }

    void transport::send_async(cl::buffer &&message,
                               send_callback_t &&callback)
    {
        assert(callback);

        m_pipe_endpoint->write_async(std::move(message),
                                     std::move(callback));
    }

    void transport::recv_async(recv_callback_t &&callback)
    {
        assert(callback);

        m_pipe_endpoint->read_async(std::move(callback));
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
