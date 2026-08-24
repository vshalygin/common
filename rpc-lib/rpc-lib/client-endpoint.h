#pragma once
#include <rpc-lib/types.h>
#include <rpc-lib/internal/endpoint/endpoint.h>
#include <rpc-lib/internal/connector/client-connector.h>
#include <rpc-lib/internal/service/service.h>

#include <common-lib/thread/thread-pool/thread-pool.h>

#include <memory>
#include <mutex>

namespace vshalygin::rpc {
    class iauthenticator;
    class iclient_pipe_env;

    template<typename GServerServiceStub, typename GClientService>
    class client_endpoint
    {
    public:
        using disconnect_future = future<void>;
        using connect_future = future<ftuple<disconnect_future>>;

        template<typename Response>
        using request_future = future<ftuple<request_result, std::unique_ptr<Response>>>;

        explicit client_endpoint(cl::thread_pool *thread_pool,
                                 std::shared_ptr<iauthenticator> authenticator,
                                 std::shared_ptr<iclient_pipe_env> pipe_env,
                                 std::shared_ptr<GClientService> gservice,
                                 const config & = config{});

        client_endpoint(const client_endpoint &) = delete;
        client_endpoint &operator=(const client_endpoint &) = delete;

        auto connect_async(std::chrono::milliseconds timeout);
        bool is_connected() const;
        void disconnect();

        template<typename Request, typename Response, typename StubMethod>
        request_future<Response> make_request(StubMethod stub_method,
                                              const Request &req);

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };

    template<typename GServerServiceStub, typename GClientService>
    class client_endpoint<GServerServiceStub, GClientService>::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        explicit impl(cl::thread_pool *thread_pool,
                      std::shared_ptr<iauthenticator> authenticator,
                      std::shared_ptr<iclient_pipe_env> pipe_env,
                      std::shared_ptr<GClientService> gservice,
                      const config &config);

        impl(const impl &) = delete;
        impl &operator=(const impl &) = delete;

        auto connect_async(std::chrono::milliseconds timeout);
        bool is_connected() const;
        void disconnect();

        template<typename Request, typename Response, typename StubMethod>
        request_future<Response> make_request(StubMethod stub_method,
                                              const Request &req);

    private:
        auto establish_endpoint(std::unique_ptr<internal::iconnection> &&connection);

    private:
        cl::thread_pool *m_thread_pool;
        std::shared_ptr<GClientService> m_gservice;

        internal::client_connector m_client_connector;

        mutable std::mutex m_mtx;
        std::unique_ptr<internal::endpoint<GServerServiceStub>> m_endpoint;
    };

    template<typename GServieServiceStub, typename GClientService>
    client_endpoint<GServieServiceStub, GClientService>::impl::impl(cl::thread_pool *thread_pool,
                                                                    std::shared_ptr<iauthenticator> authenticator,
                                                                    std::shared_ptr<iclient_pipe_env> pipe_env,
                                                                    std::shared_ptr<GClientService> gservice,
                                                                    const config &config)
        : m_thread_pool(thread_pool)
        , m_gservice(std::move(gservice))
        , m_client_connector(m_thread_pool, authenticator, pipe_env, config)
    {}

    template<typename GServieServiceStub, typename GClientService>
    template<typename Request, typename Response, typename StubMethod>
    client_endpoint<GServieServiceStub, GClientService>::request_future<Response>
        client_endpoint<GServieServiceStub, GClientService>::impl::make_request(StubMethod stub_method,
                                                                                const Request &req)
    {
        std::lock_guard guard(m_mtx);
        if(!m_endpoint || !m_endpoint->is_connected()) {
            promise p(m_thread_pool, [](request_result r, std::unique_ptr<Response> m) {
                return ftuple{ r, std::move(m) };
            });
            p.resolve(request_result::no_connection, {});
            return p.get_future();
        }

        return m_endpoint->template make_request<Request, Response, StubMethod>(stub_method, req);
    }

    template<typename GServerServiceStub, typename GClientService>
    auto client_endpoint<GServerServiceStub, GClientService>::impl::connect_async(std::chrono::milliseconds timeout)
    {
        auto f = m_client_connector.create_connection_async(
                                 std::make_shared<internal::service<GClientService>>(m_gservice, m_thread_pool, 0),
                                 timeout);

        return f.then([self = this->weak_from_this()](std::unique_ptr<internal::iconnection> &&connection) {
            std::shared_ptr s(self);
            return ftuple(s->establish_endpoint(std::move(connection)));
        });
    }

    template<typename GServerServiceStub, typename GClientService>
    bool client_endpoint<GServerServiceStub, GClientService>::impl::is_connected() const
    {
        std::lock_guard guard(m_mtx);
        return m_endpoint && m_endpoint->is_connected();
    }

    template<typename GServerServiceStub, typename GClientService>
    void client_endpoint<GServerServiceStub, GClientService>::impl::disconnect()
    {
        std::lock_guard guard(m_mtx);
        if(m_endpoint) {
            m_endpoint->disconnect();
            m_endpoint.reset();
        }
    }

    template<typename GServerServiceStub, typename GClientService>
    auto client_endpoint<GServerServiceStub, GClientService>::impl::establish_endpoint(
        std::unique_ptr<internal::iconnection> &&c)
    {
        promise disconnect_promise(m_thread_pool, []() {});
        auto disconnect_future = disconnect_promise.get_future();

        std::lock_guard guard(m_mtx);
        m_endpoint = std::make_unique<internal::endpoint<GServerServiceStub>>(std::move(c), m_thread_pool);
        m_endpoint->set_disconnect_callback(
            cl::thread_pool_task(
                m_thread_pool,
                [disconnect_promise = std::move(disconnect_promise)]() mutable {
                    disconnect_promise.resolve();
                }));

        m_endpoint->start();

        return disconnect_future;
    }

    template<typename GServerServiceStub, typename GClientService>
    client_endpoint<GServerServiceStub, GClientService>::client_endpoint(cl::thread_pool *thread_pool,
                                                                         std::shared_ptr<iauthenticator> authenticator,
                                                                         std::shared_ptr<iclient_pipe_env> pipe_env,
                                                                         std::shared_ptr<GClientService> gservice,
                                                                         const config &config)
        : m_impl(std::make_shared<impl>(thread_pool, authenticator, pipe_env, gservice, config))
    {}

    template<typename GServerServiceStub, typename GClientService>
    auto client_endpoint<GServerServiceStub, GClientService>::connect_async(std::chrono::milliseconds timeout)
    {
        return m_impl->connect_async(timeout);
    }

    template<typename GServerServiceStub, typename GClientService>
    bool client_endpoint<GServerServiceStub, GClientService>::is_connected() const
    {
        return m_impl->is_connected();
    }

    template<typename GServerServiceStub, typename GClientService>
    void client_endpoint<GServerServiceStub, GClientService>::disconnect()
    {
        m_impl->disconnect();
    }

    template<typename GServerServiceStub, typename GClientService>
    template<typename Request, typename Response, typename StubMethod>
    client_endpoint<GServerServiceStub, GClientService>::request_future<Response>
        client_endpoint<GServerServiceStub, GClientService>::make_request(StubMethod stub_method,
                                                                          const Request &req)
    {
        return m_impl->template make_request<Request, Response, StubMethod>(stub_method, req);
    }
}
