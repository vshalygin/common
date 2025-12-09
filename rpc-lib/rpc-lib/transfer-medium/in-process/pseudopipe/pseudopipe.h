#pragma once
#include <common-lib/utils/buffer/buffer.h>
#include <common-lib/thread-pool/ithread-pool.h>
#include <common-lib/syncronization/guarded-value/guarded-value.h>

#include <functional>
#include <memory>
#include <queue>

namespace vsh::rpc {
    class pseudopipe final
        : public std::enable_shared_from_this<pseudopipe>
    {
        class creator
        {};

    public:
        static std::shared_ptr<pseudopipe> create(std::shared_ptr<cl::ithread_pool> thread_pool);

        pseudopipe(std::shared_ptr<cl::ithread_pool> thread_pool, creator);

        pseudopipe(pseudopipe &) = delete;
        pseudopipe &operator=(pseudopipe &) = delete;

        void write_to_client_async(cl::buffer &&buffer);
        void write_to_server_async(cl::buffer &&buffer);

        void read_from_client_async(std::function<void(cl::buffer &&)> &&callback);
        void read_from_server_async(std::function<void(cl::buffer &&)> &&callback);

    private:
        void write_to_client_message_queue(cl::buffer &&buffer);
        void write_to_server_message_queue(cl::buffer &&buffer);

        void read_from_client_message_queue();
        void read_from_server_message_queue();

        void post_write_to_client_message_queue(cl::buffer &&buffer);
        void post_write_to_server_message_queue(cl::buffer &&buffer);

        void post_read_from_client_message_queue();
        void post_read_from_server_message_queue();

    private:
        std::shared_ptr<cl::ithread_pool> m_thread_pool;
        std::unique_ptr<cl::istrand> m_strand_from_client_to_server;
        std::unique_ptr<cl::istrand> m_strand_from_server_to_client;

        cl::guarded_value<std::queue<cl::buffer>> m_client_message_queue;
        cl::guarded_value<std::queue<cl::buffer>> m_server_message_queue;

        cl::guarded_value<std::queue<std::function<void(cl::buffer &&)>>> m_read_from_client_callbacks_queue;
        cl::guarded_value<std::queue<std::function<void(cl::buffer &&)>>> m_read_from_server_callbacks_queue;
    };
}
