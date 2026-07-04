#pragma once
#include "../pipe-op-res.h"
#include <rpc-lib/types/future.h>

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/thread/thread-pool/strand.h>
#include <common-lib/utils/buffer.h>

#include <memory>
#include <queue>
#include <string>

namespace vshalygin::rpc {
    class mem_buffer final
        : public std::enable_shared_from_this<mem_buffer>
    {
        explicit mem_buffer(std::shared_ptr<cl::thread_pool> thread_pool);

    public:
        using read_promise = promise<ftuple<pipe_op_res, cl::buffer>, pipe_op_res, cl::buffer>;
        using read_future = future<ftuple<pipe_op_res, cl::buffer>>;
        using write_future = future<pipe_op_res>;

        static std::shared_ptr<mem_buffer> create(std::shared_ptr<cl::thread_pool> thread_pool);

        mem_buffer(mem_buffer &) = delete;
        mem_buffer &operator=(mem_buffer &) = delete;

        ~mem_buffer();

        future<pipe_op_res> write_async(cl::buffer &&data);
        future<ftuple<pipe_op_res, cl::buffer>> read_async();

        void invalidate();
        bool is_valid() const;

        size_t get_pending_messages_count() const;
        size_t get_pending_read_handlers_count() const;

    private:
        pipe_op_res write(cl::buffer &&data);
        void read(read_promise promise);

        void resolve_read_promises();

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;

        mutable std::mutex m_mtx;
        bool m_is_valid = true;

        std::queue<cl::buffer> m_buffer;
        std::queue<read_promise> m_read_promises;
    };

}
