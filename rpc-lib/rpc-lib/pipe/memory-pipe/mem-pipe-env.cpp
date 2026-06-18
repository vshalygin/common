#include "mem-pipe-env.h"
#include "mem-pipe.h"

namespace vshalygin::rpc {
    mem_pipe_env::mem_pipe_env(std::shared_ptr<cl::thread_pool> thread_pool)
        : m_thread_pool(std::move(thread_pool))
    {}

    std::shared_ptr<ipipe> mem_pipe_env::create_pipe()
    {
        return create_new_pipe_end(true, m_server_side_pipes, m_client_side_pipes);
    }

    std::shared_ptr<ipipe> mem_pipe_env::open_pipe()
    {
        return create_new_pipe_end(false, m_client_side_pipes, m_server_side_pipes);
    }

    std::shared_ptr<ipipe> mem_pipe_env::create_new_pipe_end(bool is_server,
                                                             queue_t &own_queue,
                                                             queue_t &corresponding_queue)
    {
        std::shared_ptr<mem_pipe> ans(new mem_pipe(is_server, m_thread_pool));
        std::shared_ptr<mem_pipe> corresponding_pipe;

        std::lock_guard guard(m_mtx);
        while(!corresponding_queue.empty() && !corresponding_pipe) {
            corresponding_pipe = corresponding_queue.front().lock();
            corresponding_queue.pop();
        }

        if(corresponding_pipe) {
            auto buffers = std::make_shared<mem_buffers>(m_thread_pool);
            ans->set_buffers(buffers);
            corresponding_pipe->set_buffers(std::move(buffers));
        } else {
            own_queue.push(ans);
        }

        return ans;
    }

    size_t mem_pipe_env::get_client_pipe_queue_size() const
    {
        std::lock_guard guard(m_mtx);
        return m_client_side_pipes.size();
    }

    size_t mem_pipe_env::get_server_pipe_queue_size() const
    {
        std::lock_guard guard(m_mtx);
        return m_server_side_pipes.size();
    }
}
