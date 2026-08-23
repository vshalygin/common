#pragma once
#include "mem-buffer.h"

#include <common-lib/thread/thread-pool/thread-pool-task.h>

#include <memory>
#include <mutex>
#include <vector>

namespace vshalygin::rpc {
    class mem_buffers
    {
    public:
        using read_future = mem_buffer::read_future;
        using write_future = mem_buffer::write_future;

        explicit mem_buffers(cl::thread_pool *thread_pool);

        mem_buffers(const mem_buffers &) = delete;
        mem_buffers &operator=(const mem_buffers &) = delete;

        ~mem_buffers();

        read_future read_async_from_server(const std::optional<std::chrono::milliseconds> &timeout);
        read_future read_async_from_client(const std::optional<std::chrono::milliseconds> &timeout);

        write_future write_async_to_client(cl::buffer &&msg,
                                           const std::optional<std::chrono::milliseconds> &timeout);
        write_future write_async_to_server(cl::buffer &&msg,
                                           const std::optional<std::chrono::milliseconds> &timeout);

        void set_invalidate_callback(cl::thread_pool_task<void()> &&callback);

        void invalidate(bool by_server);
        bool is_valid() const;

        size_t get_invalidate_callbacks_count() const;

    private:
        void invalidate_impl(bool cancel_server_side, bool cancel_client_side);

    private:
        cl::thread_pool *m_thread_pool;

        std::shared_ptr<mem_buffer> m_client_to_server;
        std::shared_ptr<mem_buffer> m_server_to_client;

        mutable std::mutex m_mtx;
        bool m_invalidated = false;
        std::vector<cl::thread_pool_task<void()>> m_on_invalidate;
    };
}
