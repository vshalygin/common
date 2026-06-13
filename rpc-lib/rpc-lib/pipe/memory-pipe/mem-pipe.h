#pragma once
#include "../ipipe.h"
#include "mem-buffer.h"

#include <common-lib/thread-pool/thread-pool.h>

#include <memory>

namespace vshalygin::rpc {
    class mem_pipe_env;

    struct mem_buffers
    {
        mem_buffers(std::shared_ptr<cl::thread_pool> thread_pool)
            : client_to_server(thread_pool)
            , server_to_client(std::move(thread_pool))
        {}

        mem_buffer client_to_server;
        mem_buffer server_to_client;
    };

    class mem_pipe
        : public ipipe
    {
        friend mem_pipe_env;

    protected:
        explicit mem_pipe(bool is_server);
        void set_buffers(std::shared_ptr<mem_buffers> mem_buffers);

    public:
        mem_pipe(mem_pipe &) = delete;
        mem_pipe &operator=(mem_pipe &) = delete;

        ~mem_pipe() override;

        [[nodiscard]] bool is_connected() const override;

        bool wait_connect_for(const std::chrono::microseconds &mcs) const override;
        bool wait_connect() const override;

        bool write_async(cl::buffer &&msg, std::function<void(pipe_op_res)> &&handler) override;
        bool read_async(std::function<void(pipe_op_res, cl::buffer &&)> &&handler) override;

        bool try_to_write_for(cl::buffer &&msg, const std::chrono::microseconds &timeout) override;
        std::optional<cl::buffer> try_to_read_for(const std::chrono::microseconds &timeout) override;

        void invalidate() override;

    private:
        mutable std::mutex mtx_;
        mutable std::condition_variable cv_;
        mutable bool stop_flag_ = false;

        std::shared_ptr<mem_buffers> mem_buffers_;

        const bool is_server_;
    };
}
