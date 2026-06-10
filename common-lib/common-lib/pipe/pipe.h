#pragma once
#include "pipe-buffer/pipe-buffer.h"

#include <memory>
#include <chrono>

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

        bool write_async(buffer &&msg, std::function<void(pipe_op_res)> &&handler);
        bool read_async(std::function<void(pipe_op_res, buffer &&)> &&handler);

        void disconnect();

    private:
        mutable std::mutex mtx_;
        mutable std::condition_variable cv_;
        std::shared_ptr<pipe_buffers> pipe_buffers_;

        const bool is_server_;
    };
}
