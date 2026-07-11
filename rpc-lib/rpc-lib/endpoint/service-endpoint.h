#pragma once
#include "endpoint.h"
#include <rpc-lib/types/connection-state.h>
#include <rpc-lib/types/future.h>
#include <rpc-lib/connector/server-connector.h>
#include <rpc-lib/pipe/iserver-pipe-env.h>
#include <rpc-lib/service/service.h>

#include <common-lib/thread/thread-pool/thread-pool.h>

#include <functional>
#include <memory>
#include <vector>
#include <utility>
#include <unordered_map>

namespace vshalygin::rpc {
    enum class server_endpoint_state
    {
        start_listening,
        stop_listening
    };

    template<typename GClientServiceStub, typename GServerService>
    class service_endpoint
    {
    public:
        template<typename Response>
        using request_future = future<ftuple<request_result, std::unique_ptr<Response>>>;

        using on_connection_change_t = std::function<void(uint64_t, connection_state)>;

        explicit service_endpoint(on_connection_change_t  &&on_connection_change,
                                  std::function<void(server_endpoint_state)> &&on_change_state,
                                  std::shared_ptr<cl::thread_pool> thread_pool,
                                  std::shared_ptr<iauthenticator> authenticator,
                                  std::shared_ptr<iserver_pipe_env> pipe_env,
                                  std::shared_ptr<GServerService> gservice,
                                  std::chrono::milliseconds handshake_timeout = std::chrono::seconds(2),
                                  std::chrono::milliseconds send_timeout = std::chrono::seconds(2),
                                  std::chrono::milliseconds recv_timeout = std::chrono::seconds(10));

        service_endpoint(const service_endpoint &) = delete;
        service_endpoint &operator=(const service_endpoint &) = delete;

        ~service_endpoint();

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

    template<typename GClientServiceStub, typename GServerService>
    class service_endpoint<GClientServiceStub, GServerService>::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        explicit impl(std::shared_ptr<cl::thread_pool> thread_pool,
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

        void set_connector(std::unique_ptr<server_connector> connector);

        void process_new_connection(uint64_t id, std::unique_ptr<iconnection> connection);

        void finalize();

        size_t get_active_connections_count() const;

    private:
        void remove_endpoint_from_map(uint64_t id);

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;

        std::shared_ptr<on_connection_change_t> m_on_connection_change;

        std::unique_ptr<server_connector> m_connector;

        mutable std::mutex m_mtx;
        std::unordered_map<uint64_t, std::unique_ptr<endpoint<GClientServiceStub>>> m_endpoints_map;
        bool m_is_finalizing = false;
    };

    template<typename GClientServiceStub, typename GServerService>
    service_endpoint<GClientServiceStub, GServerService>::impl::impl(
        std::shared_ptr<cl::thread_pool> thread_pool,
        on_connection_change_t &&on_connection_change)
        : m_thread_pool(thread_pool)
        , m_on_connection_change(std::make_shared<on_connection_change_t>(std::move(on_connection_change)))
    {}

    template<typename GClientServiceStub, typename GServerService>
    void service_endpoint<GClientServiceStub, GServerService>::impl::set_connector(
        std::unique_ptr<server_connector> connector)
    {
        m_connector = std::move(connector);
    }

    template<typename GClientServiceStub, typename GServerService>
    void service_endpoint<GClientServiceStub, GServerService>::impl::process_new_connection(
        uint64_t id, std::unique_ptr<iconnection> c)
    {
        auto disconnect_promise = make_promise(m_thread_pool.get(), []() {});
        auto disconnect_future = disconnect_promise.get_future();

        std::lock_guard guard(m_mtx);
        if(m_is_finalizing) {
            return;
        }

        auto ep = std::make_unique<endpoint<GClientServiceStub>>(std::move(c), m_thread_pool);
        ep->set_disconnect_callback( //TODO use move_only_function or something
            [disconnect_promise = std::move(disconnect_promise)]() mutable {
                disconnect_promise.resolve();
            });

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

    template<typename GClientServiceStub, typename GServerService>
    void service_endpoint<GClientServiceStub, GServerService>::impl::finalize()
    {
        std::lock_guard guard(m_mtx);
        m_is_finalizing = true;
        for(auto &el : m_endpoints_map) {
            el.second->disconnect();
        }
    }

    template<typename GClientServiceStub, typename GServerService>
    size_t service_endpoint<GClientServiceStub, GServerService>::impl::get_active_connections_count() const
    {
        std::lock_guard guard(m_mtx);
        return m_endpoints_map.size();
    }

    template<typename GClientServiceStub, typename GServerService>
    void service_endpoint<GClientServiceStub, GServerService>::impl::remove_endpoint_from_map(uint64_t id)
    {
        std::unique_ptr<endpoint<GClientServiceStub>> endpoint_to_remove;

        std::lock_guard guard(m_mtx);
        auto it = m_endpoints_map.find(id);
        if(it != m_endpoints_map.end()) {
            endpoint_to_remove = std::move(it->second);
            m_endpoints_map.erase(it);
        }
    }

    template<typename GClientServiceStub, typename GServerService>
    void service_endpoint<GClientServiceStub, GServerService>::impl::start_listening()
    {
        m_connector->start();
    }

    template<typename GClientServiceStub, typename GServerService>
    void service_endpoint<GClientServiceStub, GServerService>::impl::stop_listening()
    {
        m_connector->stop();
    }

    template<typename GClientServiceStub, typename GServerService>
    bool service_endpoint<GClientServiceStub, GServerService>::impl::is_listening() const
    {
        return m_connector->is_active();
    }

    template<typename GClientServiceStub, typename GServerService>
    template<typename Request, typename Response, typename StubMethod>
    auto service_endpoint<GClientServiceStub, GServerService>::impl::make_request(
          uint64_t connection_id, StubMethod stub_method, const Request &req)
    {
        std::lock_guard guard(m_mtx);
        auto it = m_endpoints_map.find(connection_id);
        if(it == m_endpoints_map.end() || !it->second->is_connected()) {
            auto promise = make_promise(m_thread_pool.get(), [](request_result r, std::unique_ptr<Response> m) {
                return ftuple{ r, std::move(m) };
            });
            promise.resolve(request_result::no_connection, {});
            return promise.get_future();
        }

        return it->second->template make_request<Request, Response, StubMethod>(stub_method, req);
    }

    template<typename GClientServiceStub, typename GServerService>
    template<typename Request, typename Response, typename StubMethod>
    auto service_endpoint<GClientServiceStub, GServerService>::impl::make_request_all(
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

    template<typename GClientServiceStub, typename GServerService>
    service_endpoint<GClientServiceStub, GServerService>::service_endpoint(
        on_connection_change_t &&on_connection_change,
        std::function<void(server_endpoint_state)> &&on_change_state,
        std::shared_ptr<cl::thread_pool> thread_pool,
        std::shared_ptr<iauthenticator> authenticator,
        std::shared_ptr<iserver_pipe_env> pipe_env,
        std::shared_ptr<GServerService> gservice,
        std::chrono::milliseconds handshake_timeout,
        std::chrono::milliseconds send_timeout,
        std::chrono::milliseconds recv_timeout)
        : m_impl(std::make_shared<impl>(thread_pool, std::move(on_connection_change)))
    {
        auto connector = std::make_unique<server_connector>(
            thread_pool,
            authenticator,
            pipe_env,
            [gservice, thread_pool](uint64_t id) {
                return std::make_unique<service<GServerService>>(gservice, thread_pool, id);
            },
            [impl = std::weak_ptr(m_impl)](uint64_t id, std::unique_ptr<iconnection> c) {
                if(auto self = impl.lock()) {
                    self->process_new_connection(id, std::move(c));
                }
            },
            [on_change_state = std::move(on_change_state)](server_connector_state s) {
                if(s == server_connector_state::started){
                    on_change_state(server_endpoint_state::start_listening);
                } else if (s == server_connector_state::stopped) {
                    on_change_state(server_endpoint_state::stop_listening);
                } else {
                    assert(!"unknown server_connector_state");
                }
            },
            handshake_timeout,
            send_timeout,
            recv_timeout);

        m_impl->set_connector(std::move(connector));
    }

    template<typename GClientServiceStub, typename GServerService>
    service_endpoint<GClientServiceStub, GServerService>::~service_endpoint()
    {
        m_impl->finalize();
    }

    template<typename GClientServiceStub, typename GServerService>
    void service_endpoint<GClientServiceStub, GServerService>::start_listening()
    {
        m_impl->start_listening();
    }

    template<typename GClientServiceStub, typename GServerService>
    void service_endpoint<GClientServiceStub, GServerService>::stop_listening()
    {
        m_impl->stop_listening();
    }

    template<typename GClientServiceStub, typename GServerService>
    bool service_endpoint<GClientServiceStub, GServerService>::is_listening() const
    {
        return m_impl->is_listening();
    }

    template<typename GClientServiceStub, typename GServerService>
    template<typename Request, typename Response, typename StubMethod>
    auto service_endpoint<GClientServiceStub, GServerService>::make_request(
        uint64_t connection_id, StubMethod stub_method, const Request &req)
    {
        return m_impl->template make_request<Request, Response, StubMethod>(connection_id,
                                                                            stub_method,
                                                                            req);
    }

    template<typename GClientServiceStub, typename GServerService>
    template<typename Request, typename Response, typename StubMethod>
    auto service_endpoint<GClientServiceStub, GServerService>::make_request_all(
        StubMethod stub_method, const Request &req)
    {
        return m_impl->template make_request_all<Request, Response, StubMethod>(stub_method,
                                                                            req);
    }


    template<typename GClientServiceStub, typename GServerService>
    size_t service_endpoint<GClientServiceStub, GServerService>::get_active_connections_count() const
    {
        return m_impl->get_active_connections_count();
    }
}
