#include "mem-pipe-env.h"
#include "mem-pipe-endpoint.h"
#include "mem-buffers.h"

namespace vshalygin::rpc {
    mem_pipe_env::mem_pipe_env(std::shared_ptr<cl::thread_pool> thread_pool)
        : m_thread_pool(std::move(thread_pool))
    {}

    mem_pipe_env::~mem_pipe_env()
    {
        cancel_pending_client_endpoints();
        cancel_pending_server_endpoints();
    }

    mem_pipe_env::pipe_endpoint_future mem_pipe_env::create_pipe()
    {
        return create_new_pipe_end(true,
                                   m_server_side_pipe_promises,
                                   m_client_side_pipe_promises);
    }

    mem_pipe_env::pipe_endpoint_future mem_pipe_env::open_pipe()
    {
        return create_new_pipe_end(false,
                                   m_client_side_pipe_promises,
                                   m_server_side_pipe_promises);
    }

    void mem_pipe_env::cancel_pending_client_endpoints()
    {
        std::lock_guard guard(m_mtx);
        while(!m_client_side_pipe_promises.empty()) {
            m_client_side_pipe_promises.front().resolve(pipe_wait_res::canceled, {});
            m_client_side_pipe_promises.pop();
        }
    }

    void mem_pipe_env::cancel_pending_server_endpoints()
    {
        std::lock_guard guard(m_mtx);
        while(!m_server_side_pipe_promises.empty()) {
            m_server_side_pipe_promises.front().resolve(pipe_wait_res::canceled, {});
            m_server_side_pipe_promises.pop();
        }
    }

    mem_pipe_env::pipe_endpoint_future
        mem_pipe_env::create_new_pipe_end (
            bool is_server, promises_queue_t &queue, promises_queue_t &other_queue)
    {
        auto promise = make_promise(
            m_thread_pool.get(),
            [](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> p) {
                return ftuple{ r, std::move(p)};
            });
        auto future = promise.get_future();

        std::lock_guard guard(m_mtx);
        if(!other_queue.empty()) {
            auto buffers = std::make_shared<mem_buffers>(m_thread_pool);
            std::shared_ptr<mem_pipe_endpoint> ans(new mem_pipe_endpoint(is_server, buffers));
            std::shared_ptr<mem_pipe_endpoint> other_pipe(new mem_pipe_endpoint(!is_server, buffers));

            promise.resolve(pipe_wait_res::success, std::move(ans));

            other_queue.front().resolve(pipe_wait_res::success, std::move(other_pipe));
            other_queue.pop();
        } else {
            queue.push(std::move(promise));
        }

        return future;
    }

    size_t mem_pipe_env::get_pending_client_endpoints_count() const
    {
        std::lock_guard guard(m_mtx);
        return m_client_side_pipe_promises.size();
    }

    size_t mem_pipe_env::get_pending_server_endpoints_count() const
    {
        std::lock_guard guard(m_mtx);
        return m_server_side_pipe_promises.size();
    }
}
