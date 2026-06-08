#pragma once
#include "pipe-buffer/pipe-buffer.h"

#include <memory>
#include <chrono>

namespace vshalygin::cl {
    class pipe_env;

    struct pipe_buffers
    {
        std::shared_ptr<pipe_buffer> client_to_server;
        std::shared_ptr<pipe_buffer> server_to_client;
    };

    class pipe final
    {
        friend pipe_env;

        explicit pipe(bool is_server);

        void set_buffers(std::shared_ptr<pipe_buffers> pipe_buffers);

    public:
        pipe(pipe &) = delete;
        pipe &operator=(pipe &) = delete;

        ~pipe();

        [[nodiscard]] bool is_connected() const;

        bool wait_connect_for(const std::chrono::microseconds &mcs) const;

        bool write_async(std::string &&msg, std::function<void(bool)> &&handler);
        bool read_async(std::function<void(bool, std::string &&)> &&handler);

    private:
        mutable std::mutex mtx_;
        mutable std::condition_variable cv_;
        std::shared_ptr<pipe_buffers> pipe_buffers_;

        const bool is_server_;
    };
}
