#pragma once
#include <rpc-lib/types.h>
#include <rpc-lib/pipe/iserver-pipe-env.h>
#include <rpc-lib/internal/service/service.h>
#include <rpc-lib/internal/endpoint/endpoint.h>
#include <rpc-lib/internal/connector/server-connector.h>

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/thread/thread-pool/strand.h>

#include <memory>
#include <vector>
#include <utility>
#include <unordered_map>

namespace vshalygin::rpc {
    enum class connection_state
    {
        connected,
        disconnected
    };

    enum class server_endpoint_state
    {
        start_listening,
        stop_listening
    };

    template<typename RemoteStub, typename LocalService>
    class server_endpoint
    {
    public:
        template<typename Response>
        using request_future = cl::future<cl::thread_pool, cl::ftuple<request_result, std::unique_ptr<Response>>>;

        using on_connection_change_t = std::function<void(uint64_t, connection_state)>;
        using on_change_state_t = std::function<void(server_endpoint_state)>;

        explicit server_endpoint(on_connection_change_t  &&on_connection_change,
                                 on_change_state_t &&on_change_state,
                                 cl::thread_pool *thread_pool,
                                 std::shared_ptr<iauthenticator> authenticator,
                                 std::shared_ptr<iserver_pipe_env> pipe_env,
                                 std::shared_ptr<LocalService> gservice,
                                 const config & = config{});

        server_endpoint(const server_endpoint &) = delete;
        server_endpoint &operator=(const server_endpoint &) = delete;

        ~server_endpoint();

        void start_listening();
        void stop_listening();
        bool is_listening() const;

        template<typename Request, typename Response, typename StubMethod>
        auto make_request(uint64_t connection_id, StubMethod stub_method, const Request &req);

        template<typename Request, typename Response, typename StubMethod>
        auto make_request_all(StubMethod stub_method, const Request &req);

        size_t get_active_connections_count() const;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };

    template<typename RemoteStub, typename LocalService>
    class server_endpoint<RemoteStub, LocalService>::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        explicit impl(cl::thread_pool *thread_pool,
                      on_connection_change_t &&on_connection_change);

        impl(const impl &) = delete;
        impl &operator=(const impl &) = delete;

        void start_listening();
        void stop_listening();
        bool is_listening() const;

        template<typename Request, typename Response, typename StubMethod>
        auto make_request(uint64_t connection_id, StubMethod stub_method, const Request &req);

        template<typename Request, typename Response, typename StubMethod>
        auto make_request_all(StubMethod stub_method, const Request &req);

        void set_connector(std::unique_ptr<internal::server_connector> connector);

        void process_new_connection(uint64_t id, std::unique_ptr<internal::iconnection> connection);

        void finalize();

        size_t get_active_connections_count() const;

    private:
        void remove_endpoint_from_map(uint64_t id);

    private:
        cl::thread_pool *m_thread_pool;

        std::shared_ptr<on_connection_change_t> m_on_connection_change;

        std::unique_ptr<internal::server_connector> m_connector;

        mutable std::mutex m_mtx;
        std::unordered_map<uint64_t, std::unique_ptr<internal::endpoint<RemoteStub>>> m_endpoints_map;
        bool m_is_finalizing = false;
    };

    template<typename RemoteStub, typename LocalService>
    server_endpoint<RemoteStub, LocalService>::impl::impl(
        cl::thread_pool *thread_pool,
        on_connection_change_t &&on_connection_change)
        : m_thread_pool(thread_pool)
        , m_on_connection_change(std::make_shared<on_connection_change_t>(std::move(on_connection_change)))
    {}

    template<typename RemoteStub, typename LocalService>
    void server_endpoint<RemoteStub, LocalService>::impl::set_connector(
        std::unique_ptr<internal::server_connector> connector)
    {
        m_connector = std::move(connector);
    }

    template<typename RemoteStub, typename LocalService>
    void server_endpoint<RemoteStub, LocalService>::impl::process_new_connection(
        uint64_t id, std::unique_ptr<internal::iconnection> c)
    {
        cl::promise disconnect_promise(m_thread_pool, []() {});
        auto disconnect_future = disconnect_promise.get_future();

        std::lock_guard guard(m_mtx);
        if(m_is_finalizing) {
            return;
        }

        auto ep = std::make_unique<internal::endpoint<RemoteStub>>(std::move(c), m_thread_pool);
        ep->set_disconnect_callback(
            cl::thread_pool_task(
                m_thread_pool,
                [disconnect_promise = std::move(disconnect_promise)]() mutable {
                    disconnect_promise.resolve();
                }));

        ep->start();

        m_endpoints_map.insert({ id, std::move(ep) });

        auto strand = cl::strand(m_thread_pool->get_io_context());
        strand.post([id, on_connection_change = m_on_connection_change]() {
            (*on_connection_change)(id, connection_state::connected);
        });
        disconnect_future
            .then([id, strand = std::move(strand), self = this->weak_from_this(),
                   on_connection_change = m_on_connection_change]() {
                     if(auto s = self.lock()) {
                         s->remove_endpoint_from_map(id);
                     }
                     strand.post([id, on_connection_change](){
                         (*on_connection_change)(id, connection_state::disconnected);
                     });
                  });
    }

    template<typename RemoteStub, typename LocalService>
    void server_endpoint<RemoteStub, LocalService>::impl::finalize()
    {
        std::lock_guard guard(m_mtx);
        m_is_finalizing = true;
        for(auto &el : m_endpoints_map) {
            el.second->disconnect();
        }
    }

    template<typename RemoteStub, typename LocalService>
    size_t server_endpoint<RemoteStub, LocalService>::impl::get_active_connections_count() const
    {
        std::lock_guard guard(m_mtx);
        return m_endpoints_map.size();
    }

    template<typename RemoteStub, typename LocalService>
    void server_endpoint<RemoteStub, LocalService>::impl::remove_endpoint_from_map(uint64_t id)
    {
        std::unique_ptr<internal::endpoint<RemoteStub>> endpoint_to_remove;

        std::lock_guard guard(m_mtx);
        auto it = m_endpoints_map.find(id);
        if(it != m_endpoints_map.end()) {
            endpoint_to_remove = std::move(it->second);
            m_endpoints_map.erase(it);
        }
    }

    template<typename RemoteStub, typename LocalService>
    void server_endpoint<RemoteStub, LocalService>::impl::start_listening()
    {
        m_connector->start();
    }

    template<typename RemoteStub, typename LocalService>
    void server_endpoint<RemoteStub, LocalService>::impl::stop_listening()
    {
        m_connector->stop();
    }

    template<typename RemoteStub, typename LocalService>
    bool server_endpoint<RemoteStub, LocalService>::impl::is_listening() const
    {
        return m_connector->is_active();
    }

    template<typename RemoteStub, typename LocalService>
    template<typename Request, typename Response, typename StubMethod>
    auto server_endpoint<RemoteStub, LocalService>::impl::make_request(
          uint64_t connection_id, StubMethod stub_method, const Request &req)
    {
        std::lock_guard guard(m_mtx);
        auto it = m_endpoints_map.find(connection_id);
        if(it == m_endpoints_map.end() || !it->second->is_connected()) {
            return cl::future(m_thread_pool, cl::ftuple(request_result::no_connection, std::unique_ptr<Response>{}));
        }

        return it->second->template make_request<Request, Response, StubMethod>(stub_method, req);
    }

    template<typename RemoteStub, typename LocalService>
    template<typename Request, typename Response, typename StubMethod>
    auto server_endpoint<RemoteStub, LocalService>::impl::make_request_all(
            StubMethod stub_method, const Request &req)
    {
        std::vector<std::pair<uint64_t, request_future<Response>>> ans;

        std::lock_guard guard(m_mtx);
        for(auto &el : m_endpoints_map) {
            auto f = el.second->template make_request<Request, Response, StubMethod>(stub_method, req);
            ans.push_back({ el.first, std::move(f) });
        }

        return ans;
    }

    template<typename RemoteStub, typename LocalService>
    server_endpoint<RemoteStub, LocalService>::server_endpoint(
        on_connection_change_t &&on_connection_change,
        on_change_state_t &&on_change_state,
        cl::thread_pool *thread_pool,
        std::shared_ptr<iauthenticator> authenticator,
        std::shared_ptr<iserver_pipe_env> pipe_env,
        std::shared_ptr<LocalService> gservice,
        const config &config)
        : m_impl(std::make_shared<impl>(thread_pool, std::move(on_connection_change)))
    {
        auto connector = std::make_unique<internal::server_connector>(
            thread_pool,
            authenticator,
            pipe_env,
            [gservice, thread_pool](uint64_t id) {
                return std::make_unique<internal::service<LocalService>>(gservice, thread_pool, id);
            },
            [impl = std::weak_ptr(m_impl)](uint64_t id, std::unique_ptr<internal::iconnection> c) {
                if(auto self = impl.lock()) {
                    self->process_new_connection(id, std::move(c));
                }
            },
            [on_change_state = std::move(on_change_state)](internal::server_connector_state s) {
                if(s == internal::server_connector_state::started){
                    if(on_change_state) on_change_state(server_endpoint_state::start_listening);
                } else if (s == internal::server_connector_state::stopped) {
                    if(on_change_state) on_change_state(server_endpoint_state::stop_listening);
                } else {
                    assert(!"unknown server_connector_state");
                }
            },
            config);

        m_impl->set_connector(std::move(connector));
    }

    template<typename RemoteStub, typename LocalService>
    server_endpoint<RemoteStub, LocalService>::~server_endpoint()
    {
        m_impl->finalize();
    }

    template<typename RemoteStub, typename LocalService>
    void server_endpoint<RemoteStub, LocalService>::start_listening()
    {
        m_impl->start_listening();
    }

    template<typename RemoteStub, typename LocalService>
    void server_endpoint<RemoteStub, LocalService>::stop_listening()
    {
        m_impl->stop_listening();
    }

    template<typename RemoteStub, typename LocalService>
    bool server_endpoint<RemoteStub, LocalService>::is_listening() const
    {
        return m_impl->is_listening();
    }

    template<typename RemoteStub, typename LocalService>
    template<typename Request, typename Response, typename StubMethod>
    auto server_endpoint<RemoteStub, LocalService>::make_request(
        uint64_t connection_id, StubMethod stub_method, const Request &req)
    {
        return m_impl->template make_request<Request, Response, StubMethod>(connection_id,
                                                                            stub_method,
                                                                            req);
    }

    template<typename RemoteStub, typename LocalService>
    template<typename Request, typename Response, typename StubMethod>
    auto server_endpoint<RemoteStub, LocalService>::make_request_all(
        StubMethod stub_method, const Request &req)
    {
        return m_impl->template make_request_all<Request, Response, StubMethod>(stub_method,
                                                                            req);
    }


    template<typename RemoteStub, typename LocalService>
    size_t server_endpoint<RemoteStub, LocalService>::get_active_connections_count() const
    {
        return m_impl->get_active_connections_count();
    }
}
