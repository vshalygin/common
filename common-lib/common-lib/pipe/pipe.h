#pragma once
#include "pipe-buffer/pipe-buffer.h"

#include <memory>
#include <chrono>
#include <stdexcept>
#include <optional>

namespace vshalygin::cl {
    class pipe_env;

    struct pipe_buffers
    {
        pipe_buffers(std::shared_ptr<thread_pool> thread_pool)
            : client_to_server(thread_pool)
            , server_to_client(std::move(thread_pool))
        {}

        pipe_buffer client_to_server;
        pipe_buffer server_to_client;
    };

    class pipe
    {
        friend pipe_env;

    protected:
        explicit pipe(bool is_server);
        void set_buffers(std::shared_ptr<pipe_buffers> pipe_buffers);

    public:
        pipe(pipe &) = delete;
        pipe &operator=(pipe &) = delete;

        ~pipe();

        [[nodiscard]] bool is_connected() const;

        bool wait_connect_for(const std::chrono::microseconds &mcs) const;
        bool wait_connect() const;

        bool write_async(buffer &&msg, std::function<void(pipe_op_res)> &&handler);
        bool read_async(std::function<void(pipe_op_res, buffer &&)> &&handler);

        bool try_to_write_for(buffer &&msg, const std::chrono::microseconds &timeout);
        std::optional<buffer> try_to_read_for(const std::chrono::microseconds &timeout);

        void invalidate();

    private:
        mutable std::mutex mtx_;
        mutable std::condition_variable cv_;
        mutable bool stop_flag_ = false;

        std::shared_ptr<pipe_buffers> pipe_buffers_;

        const bool is_server_;
    };
}
