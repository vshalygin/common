#include "mem-pipe-env.h"
#include "mem-pipe-endpoint.h"
#include "mem-buffers.h"

#include <common-lib/timer/multiple-timer.h>
#include <common-lib/thread/thread-pool/thread-pool.h>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

#include <mutex>
#include <optional>

namespace vshalygin::rpc {
    using pipe_endpoint_future = mem_pipe_env::pipe_endpoint_future;

    class mem_pipe_env::impl final
        : public std::enable_shared_from_this<impl>
    {
    public:
        explicit impl(cl::thread_pool *thread_pool);

        impl(impl &) = delete;
        impl &operator=(impl &) = delete;

        pipe_endpoint_future create_pipe(uint64_t client_id);
        pipe_endpoint_future create_pipe(uint64_t client_id, std::chrono::milliseconds timeout);

        pipe_endpoint_future open_pipe(uint64_t client_id);
        pipe_endpoint_future open_pipe(uint64_t client_id, std::chrono::milliseconds timeout);

        void cancel_pending_client_endpoints(const std::optional<uint64_t> &client_id);
        void cancel_pending_server_endpoints(const std::optional<uint64_t> &client_id);

        size_t get_pending_client_endpoints_count() const;
        size_t get_pending_server_endpoints_count() const;

    private:
        struct promise_data
        {
            uint64_t id;
            uint64_t client_id;
            std::optional<uint64_t> timer_id;
            pipe_endpoint_promise promise;
        };

        using promise_container = boost::multi_index::multi_index_container<
            promise_data,
            boost::multi_index::indexed_by<
            boost::multi_index::sequenced<>,
            boost::multi_index::ordered_unique<
            boost::multi_index::member<promise_data, uint64_t, &promise_data::id>>>>;

    private:
        pipe_endpoint_future create_new_pipe_end(uint64_t client_id,
                                                 bool is_server,
                                                 std::optional<std::chrono::milliseconds> timeout,
                                                 promise_container &own,
                                                 promise_container &other);

        void cancel_pending_endpoints(const std::optional<uint64_t> &client_id, promise_container &container);
        void remove_promise_by_timeout(promise_container &container, uint64_t id);

    private:
        uint64_t m_next_read_promise_id = 0;

        cl::thread_pool *m_thread_pool;
        cl::multiple_timer m_timer;

        mutable std::mutex m_mtx;
        promise_container m_client_side_pipe_promises;
        promise_container m_server_side_pipe_promises;
    };

    mem_pipe_env::impl::impl(cl::thread_pool *thread_pool)
        : m_thread_pool(thread_pool)
        , m_timer(m_thread_pool->get_io_context())
    {}

    pipe_endpoint_future mem_pipe_env::impl::create_pipe(uint64_t client_id)
    {
        return create_new_pipe_end(client_id,
                                   true,
                                   std::nullopt,
                                   m_server_side_pipe_promises,
                                   m_client_side_pipe_promises);
    }

    pipe_endpoint_future mem_pipe_env::impl::create_pipe(uint64_t client_id,
                                                         std::chrono::milliseconds timeout)
    {
        return create_new_pipe_end(client_id,
                                   true,
                                   timeout,
                                   m_server_side_pipe_promises,
                                   m_client_side_pipe_promises);
    }

    pipe_endpoint_future mem_pipe_env::impl::open_pipe(uint64_t client_id)
    {
        return create_new_pipe_end(client_id,
                                   false,
                                   std::nullopt,
                                   m_client_side_pipe_promises,
                                   m_server_side_pipe_promises);
    }

    pipe_endpoint_future mem_pipe_env::impl::open_pipe(uint64_t client_id,
                                                       std::chrono::milliseconds timeout)
    {
        return create_new_pipe_end(client_id,
                                   false,
                                   timeout, 
                                   m_client_side_pipe_promises,
                                   m_server_side_pipe_promises);
    }

    void mem_pipe_env::impl::cancel_pending_client_endpoints(const std::optional<uint64_t> &client_id)
    {
        cancel_pending_endpoints(client_id, m_client_side_pipe_promises);
    }

    void mem_pipe_env::impl::cancel_pending_server_endpoints(const std::optional<uint64_t> &client_id)
    {
        cancel_pending_endpoints(client_id, m_server_side_pipe_promises);
    }

    pipe_endpoint_future mem_pipe_env::impl::create_new_pipe_end(uint64_t client_id,
                                                                 bool is_server,
                                                                 std::optional<std::chrono::milliseconds> timeout,
                                                                 promise_container &own,
                                                                 promise_container &other)
    {
        pipe_endpoint_promise promise_to_destroy;
        std::lock_guard guard(m_mtx);
        if(!other.empty()) {
            auto buffers = std::make_shared<mem_buffers>(m_thread_pool);
            std::shared_ptr<ipipe_endpoint> ans(new mem_pipe_endpoint(is_server, buffers));
            std::shared_ptr<mem_pipe_endpoint> other_pipe(new mem_pipe_endpoint(!is_server, buffers));

            other.modify(other.begin(), [&](promise_data &el) mutable {
                el.promise.resolve(pipe_wait_res::success, std::move(other_pipe));
                if(el.timer_id) {
                    m_timer.cancel(*el.timer_id);
                }
                promise_to_destroy = std::move(el.promise);
            });

            other.pop_front();

            return pipe_endpoint_future(
                m_thread_pool,
                cl::ftuple(pipe_wait_res::success, std::move(ans)));
        }

        cl::promise promise(
            m_thread_pool,
            [](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> p) {
                return cl::ftuple{ r, std::move(p)};
            });
        auto future = promise.get_future();

        const auto promise_id = m_next_read_promise_id++;

        std::optional<uint64_t> timer_id;
        if(timeout) {
            timer_id = m_timer.start([self = weak_from_this(), promise_id, &own]() {
                if(auto s = self.lock()) {
                    s->remove_promise_by_timeout(own, promise_id);
                }
            }, *timeout);
        }

        own.push_back(promise_data{ promise_id , client_id, timer_id, std::move(promise) });

        return future;
    }

    void mem_pipe_env::impl::cancel_pending_endpoints(const std::optional<uint64_t> &client_id,
                                                      promise_container &container)
    {
        std::vector<pipe_endpoint_promise> promises_to_destroy;

        std::lock_guard guard(m_mtx);
        auto &q = container.get<0>();
        auto it = q.begin();
        while(it != q.end()) {
            if(!client_id || it->client_id == *client_id) {
                q.modify(it, [&](promise_data &el) mutable {
                    if(el.timer_id) {
                        m_timer.cancel(*el.timer_id);
                    }
                    el.promise.resolve(pipe_wait_res::canceled, {});
                    promises_to_destroy.push_back((std::move(el.promise)));
                });
                it = q.erase(it);
            } else {
                ++it;
            }
        }
    }

    void mem_pipe_env::impl::remove_promise_by_timeout(promise_container &container, uint64_t id)
    {
        pipe_endpoint_promise promise_to_destroy;

        std::lock_guard guard(m_mtx);
        auto &m = container.get<1>();
        auto it = m.find(id);
        if(it != m.end()) {
            m.modify(it, [&](promise_data &el) mutable {
                el.promise.resolve(pipe_wait_res::timeout, {});
                promise_to_destroy = std::move(el.promise);
            });
            m.erase(it);
        }
    }

    size_t mem_pipe_env::impl::get_pending_client_endpoints_count() const
    {
        std::lock_guard guard(m_mtx);
        return m_client_side_pipe_promises.size();
    }

    size_t mem_pipe_env::impl::get_pending_server_endpoints_count() const
    {
        std::lock_guard guard(m_mtx);
        return m_server_side_pipe_promises.size();
    }

    mem_pipe_env::mem_pipe_env(cl::thread_pool *thread_pool)
        : m_impl(std::make_shared<impl>(thread_pool))
    {}

    mem_pipe_env::~mem_pipe_env()
    {
        cancel_all_pending_client_endpoints();
        cancel_all_pending_server_endpoints();
    }

    pipe_endpoint_future mem_pipe_env::create_pipe(uint64_t client_id)
    {
        return m_impl->create_pipe(client_id);
    }

    pipe_endpoint_future mem_pipe_env::create_pipe(uint64_t client_id, std::chrono::milliseconds timeout)
    {
        return m_impl->create_pipe(client_id, timeout);
    }

    pipe_endpoint_future mem_pipe_env::open_pipe(uint64_t client_id)
    {
        return m_impl->open_pipe(client_id);
    }

    pipe_endpoint_future mem_pipe_env::open_pipe(uint64_t client_id, std::chrono::milliseconds timeout)
    {
        return m_impl->open_pipe(client_id, timeout);
    }

    void mem_pipe_env::cancel_pending_client_endpoints(uint64_t client_id)
    {
        m_impl->cancel_pending_client_endpoints(client_id);
    }

    void mem_pipe_env::cancel_pending_server_endpoints(uint64_t client_id)
    {
        m_impl->cancel_pending_server_endpoints(client_id);
    }

    void mem_pipe_env::cancel_all_pending_client_endpoints()
    {
        m_impl->cancel_pending_client_endpoints(std::nullopt);
    }

    void mem_pipe_env::cancel_all_pending_server_endpoints()
    {
        m_impl->cancel_pending_server_endpoints(std::nullopt);
    }

    size_t mem_pipe_env::get_pending_client_endpoints_count() const
    {
        return m_impl->get_pending_client_endpoints_count();
    }

    size_t mem_pipe_env::get_pending_server_endpoints_count() const
    {
        return m_impl->get_pending_server_endpoints_count();
    }
}
