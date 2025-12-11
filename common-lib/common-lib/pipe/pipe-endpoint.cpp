#include "pipe-endpoint.h"

namespace vsh::cl {
    pipe_endpoint::pipe_endpoint(std::shared_ptr<pipe_buffer> input_buffer,
                                 std::shared_ptr<pipe_buffer> output_buffer,
                                 destruction_callback_t &&destruction_callback)
        : m_input_buffer(std::move(input_buffer))
        , m_output_buffer(std::move(output_buffer))
        , m_destruction_callback(std::move(destruction_callback))
    {
        assert(m_input_buffer);
        assert(m_output_buffer);
    }

    pipe_endpoint::~pipe_endpoint()
    {
        if(m_destruction_callback) try {
            m_destruction_callback();
        } catch(...) {
            //TODO safe log
        }
    }

    void pipe_endpoint::write_async(cl::buffer &&buf, write_callback_t &&callback)
    {
        m_output_buffer->write_async(std::move(buf), std::move(callback));
    }

    void pipe_endpoint::read_async(read_callback_t &&callback)
    {
        m_input_buffer->read_async(std::move(callback));
    }

    bool pipe_endpoint::is_connected() const
    {
        return m_output_buffer->is_enabled() && m_input_buffer->is_enabled();
    }
}
