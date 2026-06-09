#include "pipe-env.h"

namespace vshalygin::cl {
    pipe_env::pipe_env(std::shared_ptr<thread_pool> thread_pool)
        : thread_pool_(std::move(thread_pool))
    {}

    std::shared_ptr<pipe> pipe_env::create_pipe(const std::string &name)
    {
        return create_new_pipe_end(name, true, server_side_pipes_, client_side_pipes_);
    }

    std::shared_ptr<pipe> pipe_env::open_pipe(const std::string &name)
    {
        return create_new_pipe_end(name, true, client_side_pipes_, server_side_pipes_);
    }

    std::shared_ptr<pipe> pipe_env::create_new_pipe_end(const std::string &name,
                                                        bool is_server,
                                                        pipe_map &own_map,
                                                        pipe_map &corresponding_map)
    {
        std::shared_ptr<pipe> ans(new pipe(is_server));
        std::shared_ptr<pipe> corresponding_pipe;

        std::lock_guard guard(mtx_);
        auto it = corresponding_map.find(name);
        if(it != corresponding_map.end()) {
            auto &corresponding_queue = it->second;
            while(!corresponding_queue.empty()) {
                corresponding_pipe = corresponding_queue.front().lock();
                corresponding_queue.pop();
                if(corresponding_pipe) {
                    break;
                }
            }
            if(corresponding_queue.empty()) {
                corresponding_map.erase(it);
            }
        }

        if(corresponding_pipe) {
            auto buffers = std::make_shared<pipe_buffers>(thread_pool_);
            ans->set_buffers(buffers);
            corresponding_pipe->set_buffers(std::move(buffers));
        } else {
            own_map[name].push(ans);
        }

        return ans;
    }

    size_t pipe_env::get_client_pipe_map_size() const
    {
        std::lock_guard guard(mtx_);
        return client_side_pipes_.size();
    }

    size_t pipe_env::get_server_pipe_map_size() const
    {
        std::lock_guard guard(mtx_);
        return server_side_pipes_.size();
    }

    size_t pipe_env::get_client_pipe_queue_size(const std::string &name) const
    {
        std::lock_guard guard(mtx_);
        auto it = client_side_pipes_.find(name);
        return it != client_side_pipes_.end() ? it->second.size() : 0;
    }

    size_t pipe_env::get_server_pipe_queue_size(const std::string &name) const
    {
        std::lock_guard guard(mtx_);
        auto it = server_side_pipes_.find(name);
        return it != server_side_pipes_.end() ? it->second.size() : 0;
    }
}
