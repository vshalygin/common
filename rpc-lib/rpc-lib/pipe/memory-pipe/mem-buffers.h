#pragma once
#include "mem-buffer.h"
#include <memory>
#include <functional>
#include <mutex>
#include <vector>

namespace vshalygin::rpc {
    class mem_buffers
    {
    public:
        using read_callback_t = std::function<void(pipe_op_res, cl::buffer &&)>;
        using write_callback_t = std::function<void(pipe_op_res)>;

        explicit mem_buffers(std::shared_ptr<cl::thread_pool> thread_pool);

        mem_buffers(const mem_buffers &) = delete;
        mem_buffers &operator=(const mem_buffers &) = delete;

        ~mem_buffers();

        void read_async_from_server(read_callback_t &&callback);
        void read_async_from_client(read_callback_t &&callback);

        void write_async_to_client(cl::buffer &&msg, write_callback_t &&callback);
        void write_async_to_server(cl::buffer &&msg, write_callback_t &&callback);

        void set_invalidate_callback(std::function<void()> &&callback);

        void invalidate();
        bool is_valid() const;

        size_t get_invalidate_callbacks_count() const;

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;

        mem_buffer m_client_to_server;
        mem_buffer m_server_to_client;

        mutable std::mutex m_mtx;
        bool m_invalidated = false;
        std::vector<std::function<void()>> m_on_invalidate;
    };
}
