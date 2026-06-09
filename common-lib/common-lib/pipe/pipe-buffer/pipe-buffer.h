#pragma once
#include <common-lib/thread-pool/thread-pool.h>
#include <common-lib/thread-pool/strand.h>

#include <memory>
#include <queue>
#include <string>
#include <functional>

namespace vshalygin::cl {
    class pipe_buffer final
    {
    public:
        explicit pipe_buffer(std::shared_ptr<thread_pool> thread_pool);

        pipe_buffer(pipe_buffer &) = delete;
        pipe_buffer &operator=(pipe_buffer &) = delete;

        ~pipe_buffer();

        void write_async(std::string &&data, std::function<void(bool)> &&handler);
        void read_async(std::function<void(bool, std::string &&)> &&handler);

        void invalidate() noexcept;
        bool is_valid() const;

        size_t get_pending_messages_count() const;
        size_t get_pending_read_handlers_count() const;

    private:
        bool is_valid_ = true;
        strand strand_;

        std::queue<std::string> buffer_;
        std::queue<std::function<void(bool, std::string &&)>> read_handlers_;
    };

}
