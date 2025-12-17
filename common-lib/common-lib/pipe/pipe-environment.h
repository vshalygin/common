#pragma once
#include "pipe-endpoint.h"
#include "pipe-result.h"
#include "common-lib/thread-pool/thread-pool.h"

namespace vsh::cl {
    class pipe_environment final
    {
    public:
        using pipe_endpoint_sp = std::shared_ptr<pipe_endpoint>;
        using pipe_callback_t = std::function<void(pipe_result, pipe_endpoint_sp)>;

        explicit pipe_environment(std::shared_ptr<thread_pool> thread_pool);

        pipe_environment(pipe_environment &) = delete;
        pipe_environment &operator=(pipe_environment &) = delete;

        pipe_environment(pipe_environment &&);
        pipe_environment &operator=(pipe_environment &&);

        void create_pipe_async(const std::string &pipe_name,
                               pipe_callback_t &&callback,
                               const std::chrono::milliseconds &timeout);
        void open_pipe_async(const std::string &pipe_name,
                             std::function<void(pipe_result, pipe_endpoint_sp)> &&callback,
                             const std::chrono::milliseconds &timeout);

    private:
        class impl;
        std::unique_ptr<impl> m_impl;
    };
}
