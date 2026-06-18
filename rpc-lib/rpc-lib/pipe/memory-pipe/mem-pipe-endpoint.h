#pragma once
#include "../ipipe-endpoint.h"
#include "mem-buffer.h"

#include <common-lib/thread-pool/thread-pool.h>

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
                                   std::shared_ptr<cl::thread_pool> thread_pool);
        void set_buffers(std::shared_ptr<mem_buffers> mem_buffers);

    public:
        mem_pipe_endpoint(mem_pipe_endpoint &) = delete;
        mem_pipe_endpoint &operator=(mem_pipe_endpoint &) = delete;

        ~mem_pipe_endpoint() override;

        bool is_connected() const override;
        void subscribe_to_disconnect(std::function<void()> &&callback) override;

        pipe_wait_res wait_connect_for(const std::chrono::microseconds &mcs) const override;
        pipe_wait_res wait_connect() const override;

        void write_async(cl::buffer &&msg, write_callback_t &&callback) override;
        void read_async(read_callback_t &&callback) override;

        bool try_to_write_for(cl::buffer &&msg, const std::chrono::microseconds &timeout) override;
        std::optional<cl::buffer> try_to_read_for(const std::chrono::microseconds &timeout) override;

        void invalidate() override;

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;

        mutable std::mutex m_mtx;
        mutable std::condition_variable m_cv;

        bool m_is_invalidated = false;

        std::shared_ptr<mem_buffers> m_mem_buffers;

        std::function<void()> m_on_disconnect;

        const bool m_is_server;
    };
}
