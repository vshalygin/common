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
    class mem_pipe_env::impl final
        : public std::enable_shared_from_this<impl>
    {
    public:
        explicit impl(std::shared_ptr<cl::thread_pool> thread_pool);

        impl(impl &) = delete;
        impl &operator=(impl &) = delete;

        pipe_endpoint_future create_pipe();
        pipe_endpoint_future create_pipe(std::chrono::milliseconds timeout);

        pipe_endpoint_future open_pipe();
        pipe_endpoint_future open_pipe(std::chrono::milliseconds timeout);

        void cancel_pending_client_endpoints();
        void cancel_pending_server_endpoints();

        size_t get_pending_client_endpoints_count() const;
        size_t get_pending_server_endpoints_count() const;

    private:
        struct promise_data
        {
            uint64_t id;
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
        pipe_endpoint_future create_new_pipe_end(
            bool is_server, std::optional<std::chrono::milliseconds> timeout,
            promise_container &own, promise_container &other);

        void cancel_pending_endpoints(promise_container &container);
        void remove_promise_by_timeout(promise_container &container, uint64_t id);

    private:
        uint64_t m_next_read_promise_id = 0;

        std::shared_ptr<cl::thread_pool> m_thread_pool;
        cl::multiple_timer m_timer;

        mutable std::mutex m_mtx;
        promise_container m_client_side_pipe_promises;
        promise_container m_server_side_pipe_promises;
    };

    mem_pipe_env::impl::impl(std::shared_ptr<cl::thread_pool> thread_pool)
        : m_thread_pool(std::move(thread_pool))
        , m_timer(m_thread_pool->get_io_context())
    {}

    mem_pipe_env::pipe_endpoint_future mem_pipe_env::impl::create_pipe()
    {
        return create_new_pipe_end(true,
                                   std::nullopt,
                                   m_server_side_pipe_promises,
                                   m_client_side_pipe_promises);
    }

    mem_pipe_env::pipe_endpoint_future mem_pipe_env::impl::create_pipe(std::chrono::milliseconds timeout)
    {
        return create_new_pipe_end(true,
                                   timeout,
                                   m_server_side_pipe_promises,
                                   m_client_side_pipe_promises);
    }

    mem_pipe_env::pipe_endpoint_future mem_pipe_env::impl::open_pipe()
    {
        return create_new_pipe_end(false,
                                   std::nullopt,
                                   m_client_side_pipe_promises,
                                   m_server_side_pipe_promises);
    }

    mem_pipe_env::pipe_endpoint_future mem_pipe_env::impl::open_pipe(std::chrono::milliseconds timeout)
    {
        return create_new_pipe_end(false,
                                   timeout, 
                                   m_client_side_pipe_promises,
                                   m_server_side_pipe_promises);
    }

    void mem_pipe_env::impl::cancel_pending_client_endpoints()
    {
        cancel_pending_endpoints(m_client_side_pipe_promises);
    }

    void mem_pipe_env::impl::cancel_pending_server_endpoints()
    {
        cancel_pending_endpoints(m_server_side_pipe_promises);
    }

    mem_pipe_env::pipe_endpoint_future mem_pipe_env::impl::create_new_pipe_end(
            bool is_server, std::optional<std::chrono::milliseconds> timeout,
            promise_container &own, promise_container &other)
    {
        auto promise = make_promise(
            m_thread_pool.get(),
            [](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> p) {
                return ftuple{ r, std::move(p)};
            });
        auto future = promise.get_future();

        pipe_endpoint_promise promise_to_destroy;
        std::lock_guard guard(m_mtx);
        if(!other.empty()) {
            auto buffers = std::make_shared<mem_buffers>(m_thread_pool);
            std::shared_ptr<mem_pipe_endpoint> ans(new mem_pipe_endpoint(is_server, buffers));
            std::shared_ptr<mem_pipe_endpoint> other_pipe(new mem_pipe_endpoint(!is_server, buffers));

            promise.resolve(pipe_wait_res::success, std::move(ans));

            other.modify(other.begin(), [&](promise_data &el) mutable {
                el.promise.resolve(pipe_wait_res::success, std::move(other_pipe));
                if(el.timer_id) {
                    m_timer.cancel(*el.timer_id);
                }
                promise_to_destroy = std::move(el.promise);
            });

            other.pop_front();
        } else {
            const auto promise_id = m_next_read_promise_id++;

            std::optional<uint64_t> timer_id;
            if(timeout) {
                timer_id = m_timer.start([self = weak_from_this(), promise_id, &own]() {
                    if(auto s = self.lock()) {
                        s->remove_promise_by_timeout(own, promise_id);
                    }
                }, *timeout);
            }
            
            own.push_back(promise_data{ promise_id, timer_id, std::move(promise) });
        }

        return future;
    }

    void mem_pipe_env::impl::cancel_pending_endpoints(promise_container &container)
    {
        std::vector<pipe_endpoint_promise> promises_to_destroy;

        std::lock_guard guard(m_mtx);
        auto &q = container.get<0>();
        while(!q.empty()) {
            q.modify(q.begin(), [&](promise_data &el) mutable {
                el.promise.resolve(pipe_wait_res::canceled, {});
                promises_to_destroy.push_back((std::move(el.promise)));
            });
            q.pop_front();
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

    mem_pipe_env::mem_pipe_env(std::shared_ptr<cl::thread_pool> thread_pool)
        : m_impl(std::make_shared<impl>(std::move(thread_pool)))
    {}

    mem_pipe_env::~mem_pipe_env()
    {
        cancel_pending_client_endpoints();
        cancel_pending_server_endpoints();
    }

    mem_pipe_env::pipe_endpoint_future mem_pipe_env::create_pipe()
    {
        return m_impl->create_pipe();
    }

    mem_pipe_env::pipe_endpoint_future mem_pipe_env::create_pipe(std::chrono::milliseconds timeout)
    {
        return m_impl->create_pipe(timeout);
    }

    mem_pipe_env::pipe_endpoint_future mem_pipe_env::open_pipe()
    {
        return m_impl->open_pipe();
    }

    mem_pipe_env::pipe_endpoint_future mem_pipe_env::open_pipe(std::chrono::milliseconds timeout)
    {
        return m_impl->open_pipe(timeout);
    }

    void mem_pipe_env::cancel_pending_client_endpoints()
    {
        m_impl->cancel_pending_client_endpoints();
    }

    void mem_pipe_env::cancel_pending_server_endpoints()
    {
        m_impl->cancel_pending_server_endpoints();
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
