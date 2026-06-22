#pragma once
#include "../ipipe-env.h"
#include <common-lib/thread/thread-pool.h>

#include <queue>
#include <mutex>

namespace vshalygin::rpc {
    class mem_pipe_endpoint;

    class mem_pipe_env final
        : public ipipe_env
    {
        using queue_t = std::queue<std::weak_ptr<mem_pipe_endpoint>>;

    public:
        explicit mem_pipe_env(std::shared_ptr<cl::thread_pool> thread_pool);

        mem_pipe_env(mem_pipe_env &) = delete;
        mem_pipe_env &operator=(mem_pipe_env &) = delete;

        std::shared_ptr<ipipe_endpoint> create_pipe() override;
        std::shared_ptr<ipipe_endpoint> open_pipe() override;

        size_t get_client_pipe_endpoint_queue_size() const;
        size_t get_server_pipe_endpoint_queue_size() const;

    private:
        std::shared_ptr<ipipe_endpoint> create_new_pipe_end(bool is_server,
                                                            queue_t &own_queue,
                                                            queue_t &corresponding_queue);

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;

        mutable std::mutex m_mtx;
        queue_t m_client_side_pipe_endpoints;
        queue_t m_server_side_pipe_endpoints;
    };
}
