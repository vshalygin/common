#ifdef _WIN32

#include "win-pipe-operations/win-pipe-open-operation.h"
#include "win-pipe-iocp-owner.h"
#include "win-pipe-client-env.h"
#include "win-pipe-endpoint.h"

#include <common-lib/synchronization/value-locker.h>
#include <common-lib/timer/multiple-timer.h>

#include <memory>
#include <unordered_map>
#include <string>
#include <optional>


namespace vshalygin::rpc {
    using pipe_endpoint_future = win_pipe_client_env::pipe_endpoint_future;
    using op_res = internal::win_pipe_operation_res;

    class win_pipe_client_env::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        explicit impl(std::shared_ptr<cl::thread_pool> thread_pool,
                      const std::wstring &pipe_name);

        impl(const impl &) = delete;
        impl &operator=(const impl &) = delete;

        pipe_endpoint_future open_pipe(uint64_t client_id);
        pipe_endpoint_future open_pipe(uint64_t client_id, std::chrono::milliseconds timeout);

        void cancel_pending_client_endpoints(const std::optional<uint64_t> &client_id);

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;
        const std::wstring m_pipe_name;
        std::shared_ptr<internal::win_pipe_iocp_owner> m_iocp_owner;

        using open_operation = internal::win_pipe_open_operation;
        struct open_operation_info
        {
            std::unique_ptr<open_operation> op;
            uint64_t client_id;
            std::optional<uint64_t> timer_id;
        };

        using op_map = cl::value_locker<std::unordered_map<uint64_t, open_operation_info>>;
        std::shared_ptr<op_map> m_pending_open_operations;

        std::atomic<uint64_t> m_next_id = 0;
        cl::multiple_timer m_timer;
    };

    win_pipe_client_env::impl::impl(std::shared_ptr<cl::thread_pool> thread_pool,
                                    const std::wstring &pipe_name)
        : m_thread_pool(std::move(thread_pool))
        , m_pipe_name(pipe_name)
        , m_iocp_owner(internal::win_pipe_iocp_owner::create())
        , m_pending_open_operations(std::make_shared<op_map>())
        , m_timer(m_thread_pool->get_io_context())
    {}

    pipe_endpoint_future win_pipe_client_env::impl::open_pipe(uint64_t client_id)
    {
        auto id = m_next_id.fetch_add(1, std::memory_order_relaxed);

        auto pending_ops = m_pending_open_operations->lock();
        auto op = std::make_unique<open_operation>(m_pipe_name, m_thread_pool.get());
        auto f = m_iocp_owner->open_pipe_async(op.get());

        pending_ops->insert({ id, open_operation_info{ std::move(op), client_id, std::nullopt } });

        return f.finally([id, self = shared_from_this()]{ self->m_pending_open_operations->lock()->erase(id); })
                .then([self = shared_from_this()]
                      (pipe_wait_res r, win::pipe_handle &&p) {
                          std::shared_ptr<ipipe_endpoint> endpoint = is_success(r)
                              ? std::make_shared<win_pipe_endpoint>(std::move(p),
                                                                    self->m_iocp_owner,
                                                                    self->m_thread_pool)
                              : std::shared_ptr<win_pipe_endpoint>{};
                          return ftuple(r, std::move(endpoint));
                      });
    }

    pipe_endpoint_future win_pipe_client_env::impl::open_pipe(uint64_t client_id,
                                                              std::chrono::milliseconds timeout)
    {
        auto op = std::make_unique<open_operation>(m_pipe_name, m_thread_pool.get());
        auto f = m_iocp_owner->open_pipe_async(op.get());
        
        auto pending_ops = m_pending_open_operations->lock();
        auto id = m_next_id.fetch_add(1, std::memory_order_relaxed);
        auto timer_id = m_timer.start([id, self = weak_from_this()]() {
            if(auto s = self.lock()) {
                auto pending_ops = s->m_pending_open_operations->lock();
                auto it = pending_ops->find(id);
                if(it != pending_ops->end()) {
                    it->second.op->cancel(true);
                }
            }
        }, timeout);

        pending_ops->insert({ id, open_operation_info{ std::move(op), client_id, timer_id } });
        
        return f.finally([id, self = shared_from_this()]() {
                             auto ops = self->m_pending_open_operations->lock();
                             auto it = ops->find(id);
                             if(it != ops->end()) {
                                 self->m_timer.cancel(*it->second.timer_id);
                                 ops->erase(it);
                             }
                         })
                .then([self = shared_from_this()](pipe_wait_res r, win::pipe_handle &&p) {
                          std::shared_ptr<ipipe_endpoint> endpoint = is_success(r)
                              ? std::make_shared<win_pipe_endpoint>(std::move(p),
                                                                    self->m_iocp_owner,
                                                                    self->m_thread_pool)
                              : std::shared_ptr<win_pipe_endpoint>{};
                          return ftuple(r, std::move(endpoint));
                      });
    }

    void win_pipe_client_env::impl::cancel_pending_client_endpoints(const std::optional<uint64_t> &client_id)
    {
        auto pending_ops = m_pending_open_operations->lock();

        for(auto &op : *pending_ops) {
            if(!client_id || op.second.client_id == *client_id) {
                op.second.op->cancel(false);
            }
        }
    }

    win_pipe_client_env::win_pipe_client_env(std::shared_ptr<cl::thread_pool> thread_pool,
                                             const std::wstring &pipe_name)
        : m_impl(std::make_shared<impl>(std::move(thread_pool), pipe_name))
    {}

    win_pipe_client_env::~win_pipe_client_env()
    {
        cancel_all_pending_client_endpoints();
    }

    pipe_endpoint_future win_pipe_client_env::open_pipe(uint64_t client_id)
    {
        return m_impl->open_pipe(client_id);
    }

    pipe_endpoint_future win_pipe_client_env::open_pipe(uint64_t client_id,
                                                        std::chrono::milliseconds timeout)
    {
        return m_impl->open_pipe(client_id, timeout);
    }

    void win_pipe_client_env::cancel_pending_client_endpoints(uint64_t client_id)
    {
        m_impl->cancel_pending_client_endpoints(client_id);
    }

    void win_pipe_client_env::cancel_all_pending_client_endpoints()
    {
        m_impl->cancel_pending_client_endpoints(std::nullopt);
    }
}

#endif
