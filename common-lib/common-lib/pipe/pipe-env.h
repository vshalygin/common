#pragma once
#include "pipe.h"
#include <common-lib/thread-pool/thread-pool.h>

#include <memory>
#include <string>
#include <queue>
#include <mutex>
#include <map>

namespace vshalygin::cl {
    class pipe_env final
    {
        using pipe_map = std::map<std::string, std::queue<std::weak_ptr<pipe>>>;

    public:
        explicit pipe_env(std::shared_ptr<thread_pool> thread_pool);

        pipe_env(pipe_env &) = delete;
        pipe_env &operator=(pipe_env &) = delete;

        std::shared_ptr<pipe> create_pipe(const std::string &name);
        std::shared_ptr<pipe> open_pipe(const std::string &name);

        size_t get_client_pipe_map_size() const;
        size_t get_server_pipe_map_size() const;
        size_t get_client_pipe_queue_size(const std::string &name) const;
        size_t get_server_pipe_queue_size(const std::string &name) const;

    private:
        std::shared_ptr<pipe> create_new_pipe_end(const std::string &name,
                                                  bool is_server,
                                                  pipe_map &own_queue,
                                                  pipe_map &corresponding_queue);

    private:
        std::shared_ptr<thread_pool> thread_pool_;

        mutable std::mutex mtx_;
        pipe_map client_side_pipes_;
        pipe_map server_side_pipes_;
    };
}
