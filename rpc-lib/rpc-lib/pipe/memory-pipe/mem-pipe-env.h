#pragma once
#include "../ipipe-env.h"
#include <common-lib/thread-pool/thread-pool.h>

#include <queue>
#include <mutex>
#include <map>

namespace vshalygin::rpc {
    class mem_pipe;

    class mem_pipe_env final
        : public ipipe_env
    {
        using pipe_map = std::map<std::string, std::queue<std::weak_ptr<mem_pipe>>>;

    public:
        explicit mem_pipe_env(std::shared_ptr<cl::thread_pool> thread_pool);

        mem_pipe_env(mem_pipe_env &) = delete;
        mem_pipe_env &operator=(mem_pipe_env &) = delete;

        std::shared_ptr<ipipe> create_pipe(const std::string &name) override;
        std::shared_ptr<ipipe> open_pipe(const std::string &name) override;

        size_t get_client_pipe_map_size() const;
        size_t get_server_pipe_map_size() const;
        size_t get_client_pipe_queue_size(const std::string &name) const;
        size_t get_server_pipe_queue_size(const std::string &name) const;

    private:
        std::shared_ptr<ipipe> create_new_pipe_end(const std::string &name,
                                                   bool is_server,
                                                   pipe_map &own_queue,
                                                   pipe_map &corresponding_queue);

    private:
        std::shared_ptr<cl::thread_pool> thread_pool_;

        mutable std::mutex mtx_;
        pipe_map client_side_pipes_;
        pipe_map server_side_pipes_;
    };
}
