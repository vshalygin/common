#pragma once
#include "../pipe-op-res.h"

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/thread/thread-pool/strand.h>
#include <common-lib/utils/buffer/buffer.h>

#include <memory>
#include <queue>
#include <string>
#include <functional>

namespace vshalygin::rpc {
    class mem_buffer final
    {
    public:
        explicit mem_buffer(std::shared_ptr<cl::thread_pool> thread_pool);

        mem_buffer(mem_buffer &) = delete;
        mem_buffer &operator=(mem_buffer &) = delete;

        ~mem_buffer();

        void write_async(cl::buffer &&data, std::function<void(pipe_op_res)> &&callback);
        void read_async(std::function<void(pipe_op_res, cl::buffer &&)> &&callback);

        void invalidate();
        bool is_valid() const;

        size_t get_pending_messages_count() const;
        size_t get_pending_read_handlers_count() const;

    private:
        bool is_valid_ = true;
        cl::strand strand_;

        std::queue<cl::buffer> buffer_;
        std::queue<std::function<void(pipe_op_res, cl::buffer &&)>> read_handlers_;
    };

}
