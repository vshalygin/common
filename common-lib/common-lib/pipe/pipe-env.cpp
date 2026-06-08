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

        std::lock_guard guard(mtx_);
        auto &corresponding_queue = corresponding_map[name];
        while(!corresponding_queue.empty()) {
            auto client_pipe = corresponding_queue.front().lock();
            corresponding_queue.pop();
            if(client_pipe) {
                auto buffers = std::make_shared<pipe_buffers>(thread_pool_);
                ans->set_buffers(buffers);
                client_pipe->set_buffers(std::move(buffers));
                return ans;
            }
        }

        own_map[name].push(ans);
        return ans;
    }
}
