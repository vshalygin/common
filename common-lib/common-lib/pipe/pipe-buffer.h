#pragma once
#include "pipe-result.h"

#include "common-lib/utils/buffer/buffer.h"
#include "common-lib/thread-pool/thread-pool.h"
#include "common-lib/thread-pool/strand.h"

#include <functional>
#include <queue>
#include <atomic>

namespace vsh::cl {
    class pipe_buffer final
        : public std::enable_shared_from_this<pipe_buffer>
    {
        class creator
        {};

    public:
        using read_callback_t = std::function<void(pipe_result, cl::buffer &&)>;
        using write_callback_t = std::function<void(pipe_result)>;

        static std::shared_ptr<pipe_buffer> create(std::shared_ptr<cl::thread_pool> thread_pool);

        pipe_buffer(std::shared_ptr<cl::thread_pool> thread_pool,
                    creator);

        pipe_buffer(pipe_buffer &) = delete;
        pipe_buffer &operator=(pipe_buffer &) = delete;

        ~pipe_buffer();

        void write_async(cl::buffer &&buf, write_callback_t &&callback);
        void read_async(read_callback_t &&callback);

        bool is_enabled() const;
        void enable();
        void disable();

    private:
        void read_from_queue_if_possible();

    private:
        std::atomic_bool m_is_enabled = false;

        std::unique_ptr<cl::strand> m_strand;

        std::queue<cl::buffer> m_message_queue;
        read_callback_t m_read_callback;
    };
}
