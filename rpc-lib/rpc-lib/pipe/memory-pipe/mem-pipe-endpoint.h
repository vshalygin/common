#pragma once
#include "../ipipe-endpoint.h"
#include "mem-buffer.h"

#include <common-lib/thread/thread-pool/thread-pool.h>

#include <memory>

namespace vshalygin::rpc {
    class mem_pipe_env;
    class mem_buffers;

    class mem_pipe_endpoint
        : public ipipe_endpoint
    {
        friend mem_pipe_env;

    protected:
        explicit mem_pipe_endpoint(bool is_server,
                                   std::shared_ptr<mem_buffers> mem_buffers);

    public:
        mem_pipe_endpoint(mem_pipe_endpoint &) = delete;
        mem_pipe_endpoint &operator=(mem_pipe_endpoint &) = delete;

        ~mem_pipe_endpoint() override;

        bool is_connected() const override;
        void set_disconnect_callback(cl::thread_pool_task<void()> &&callback) override;

        write_future write_async(cl::buffer &&msg) override;
        read_future read_async() override;
        write_future write_async(cl::buffer &&msg, std::chrono::milliseconds timeout) override;
        read_future read_async(std::chrono::milliseconds timeout) override;

        void invalidate() override;

    private:
        write_future write_async(cl::buffer &&msg,
                                 const std::optional<std::chrono::milliseconds> &timeout);
        read_future read_async(const std::optional<std::chrono::milliseconds> &timeout);

    private:
        std::shared_ptr<mem_buffers> m_mem_buffers;
        const bool m_is_server;
    };
}
