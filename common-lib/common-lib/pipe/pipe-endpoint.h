#pragma once
#include "common-lib/pipe/pipe-buffer.h"
#include "common-lib/utils/buffer/buffer.h"

#include <memory>

namespace vshalygin::cl {
    class pipe_endpoint final
    {
    public:
        using write_callback_t = pipe_buffer::write_callback_t;
        using read_callback_t = pipe_buffer::read_callback_t;

        pipe_endpoint(std::shared_ptr<pipe_buffer> input_buffer,
                      std::shared_ptr<pipe_buffer> output_buffer);

        pipe_endpoint(pipe_endpoint &) = delete;
        pipe_endpoint &operator=(pipe_endpoint &) = delete;

        ~pipe_endpoint();

        void write_async(cl::buffer &&buf, write_callback_t &&callback);
        void read_async(read_callback_t &&callback);

        bool is_connected() const;

    private:
        std::shared_ptr<pipe_buffer> m_input_buffer;
        std::shared_ptr<pipe_buffer> m_output_buffer;
    };
}
