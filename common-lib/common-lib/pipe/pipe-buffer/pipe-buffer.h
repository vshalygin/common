#pragma once
#include <common-lib/thread-pool/thread-pool.h>
#include <common-lib/thread-pool/strand.h>

#include <memory>
#include <queue>
#include <string>
#include <functional>

namespace vshalygin::cl {
    class pipe_buffer final
        : public std::enable_shared_from_this<pipe_buffer>
    {
        class creator
        {};

    public:
        static std::shared_ptr<pipe_buffer> create(std::shared_ptr<thread_pool> thread_pool);

        pipe_buffer(std::shared_ptr<thread_pool> thread_pool, creator);

        pipe_buffer(pipe_buffer &) = delete;
        pipe_buffer &operator=(pipe_buffer &) = delete;

        ~pipe_buffer();

        void write_async(std::string &&data);
        void start_reading_async(std::function<void(std::string &&)> &&handler);

    private:
        void read_async();

    private:
        strand strand_;
        std::queue<std::string> buffer_;

        std::function<void(std::string &&)> handler_;
    };

}
