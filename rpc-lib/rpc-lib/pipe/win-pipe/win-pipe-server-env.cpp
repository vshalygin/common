#ifdef _WIN32
#include "win-pipe-server-env.h"
#include "win-pipe-iocp-owner.h"
#include "win-pipe-endpoint.h"
#include "win-pipe-operations/win-pipe-create-operation.h"

#include <common-lib/thread/thread.h>

#include <common-lib/synchronization/value-locker.h>
#include <common-lib/timer/multiple-timer.h>

#include <unordered_map>
#include <atomic>
#include <optional>

namespace vshalygin::rpc {
    using pipe_endpoint_future = win_pipe_server_env::pipe_endpoint_future;

    class win_pipe_server_env::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        explicit impl(const std::wstring &pipe_name,
                      cl::thread_pool *thread_pool);

        impl(const impl &) = delete;
        impl &operator=(const impl &) = delete;

        pipe_endpoint_future create_pipe(uint64_t client_id);
        pipe_endpoint_future create_pipe(uint64_t client_id, std::chrono::milliseconds timeout);
        void cancel_pending_server_endpoints(const std::optional<uint64_t> &client_id);

    private:
        const std::wstring m_pipe_name;
        cl::thread_pool *m_thread_pool;
        std::shared_ptr<internal::win_pipe_iocp_owner> m_iocp_owner;

        using create_operation = internal::win_pipe_create_operation;
        struct create_operation_info
        {
            std::shared_ptr<create_operation> op;
            uint64_t client_id;
            std::optional<uint64_t> timer_id;
        };

        using op_map = cl::value_locker<std::unordered_map<uint64_t, create_operation_info>>;
        std::shared_ptr<op_map> m_pending_create_operations;

        std::atomic<uint64_t> m_next_id = 0;
        cl::multiple_timer m_timer;
    };

    win_pipe_server_env::impl::impl(const std::wstring &pipe_name,
                                    cl::thread_pool *thread_pool)
        : m_pipe_name(pipe_name)
        , m_thread_pool(thread_pool)
        , m_iocp_owner(internal::win_pipe_iocp_owner::create())
        , m_pending_create_operations(std::make_shared<op_map>())
        , m_timer(m_thread_pool->get_io_context())
    {}

    pipe_endpoint_future win_pipe_server_env::impl::create_pipe(uint64_t client_id)
    {
        auto id = m_next_id.fetch_add(1, std::memory_order_relaxed);

        auto pending_ops = m_pending_create_operations->lock();
        auto op = create_operation::create(m_pipe_name, m_thread_pool);
        auto f = m_iocp_owner->create_pipe_async(op);

        pending_ops->insert({ id, create_operation_info{ std::move(op), client_id, std::nullopt } });

        return f.finally([id, self = shared_from_this()]{ self->m_pending_create_operations->lock()->erase(id); })
                .then([self = shared_from_this()](auto result) mutable {
                    return result.lock().with([&](pipe_wait_res r, win::pipe_handle &&p) {
                          std::shared_ptr<ipipe_endpoint> endpoint = is_success(r)
                              ? std::make_shared<win_pipe_endpoint>(std::move(p),
                                                                    self->m_iocp_owner,
                                                                    self->m_thread_pool)
                              : std::shared_ptr<win_pipe_endpoint>{};
                          return cl::ftuple(r, std::move(endpoint));
                      });
                });
    }

    pipe_endpoint_future win_pipe_server_env::impl::create_pipe(uint64_t client_id,
                                                                std::chrono::milliseconds timeout)
    {
        auto op = create_operation::create(m_pipe_name, m_thread_pool);
        auto f = m_iocp_owner->create_pipe_async(op);
        
        auto pending_ops = m_pending_create_operations->lock();
        auto id = m_next_id.fetch_add(1, std::memory_order_relaxed);
        auto timer_id = m_timer.start([id, self = weak_from_this()]() {
            if(auto s = self.lock()) {
                auto pending_ops = s->m_pending_create_operations->lock();
                auto it = pending_ops->find(id);
                if(it != pending_ops->end()) {
                    s->m_iocp_owner->cancel_create(it->second.op, true);
                }
            }
        }, timeout);

        pending_ops->insert({ id, create_operation_info{ std::move(op), client_id, timer_id } });
        
        return f.finally([id, self = shared_from_this()]() {
                             auto ops = self->m_pending_create_operations->lock();
                             auto it = ops->find(id);
                             if(it != ops->end()) {
                                 self->m_timer.cancel(*it->second.timer_id);
                                 ops->erase(it);
                             }
                         })
                .then([self = shared_from_this()](auto result) mutable {
                    return result.lock().with([&](pipe_wait_res r, win::pipe_handle &&p) {
                          std::shared_ptr<ipipe_endpoint> endpoint = is_success(r)
                              ? std::make_shared<win_pipe_endpoint>(std::move(p),
                                                                    self->m_iocp_owner,
                                                                    self->m_thread_pool)
                              : std::shared_ptr<win_pipe_endpoint>{};
                          return cl::ftuple(r, std::move(endpoint));
                      });
                });
    }

    void win_pipe_server_env::impl::cancel_pending_server_endpoints(const std::optional<uint64_t> &client_id)
    {
        auto pending_ops = m_pending_create_operations->lock();

        for(auto &op : *pending_ops) {
            if(!client_id || op.second.client_id == *client_id) {
                m_iocp_owner->cancel_create(op.second.op, false);
            }
        }
    }

    win_pipe_server_env::win_pipe_server_env(cl::thread_pool *thread_pool,
                                             const std::wstring &pipe_name)
        : m_impl(std::make_shared<impl>(pipe_name, thread_pool))
    {}

    win_pipe_server_env::~win_pipe_server_env()
    {
        cancel_all_pending_server_endpoints();
    }

    pipe_endpoint_future win_pipe_server_env::create_pipe(uint64_t client_id)
    {
        return m_impl->create_pipe(client_id);
    }

    pipe_endpoint_future win_pipe_server_env::create_pipe(uint64_t client_id, std::chrono::milliseconds timeout)
    {
        return m_impl->create_pipe(client_id, timeout);
    }

    void win_pipe_server_env::cancel_pending_server_endpoints(uint64_t client_id)
    {
        m_impl->cancel_pending_server_endpoints(client_id);
    }

    void win_pipe_server_env::cancel_all_pending_server_endpoints()
    {
        m_impl->cancel_pending_server_endpoints(std::nullopt);
    }
}

#endif
