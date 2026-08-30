#pragma once
#include <rpc-lib/types.h>
#include <rpc-lib/internal/endpoint/endpoint.h>
#include <rpc-lib/internal/connector/client-connector.h>
#include <rpc-lib/internal/service/service.h>

#include <common-lib/thread/thread.h>

#include <memory>
#include <mutex>

namespace vshalygin::rpc {
    class iauthenticator;
    class iclient_pipe_env;

    template<typename RemoteStub, typename LocalService>
    class client_endpoint
    {
    public:
        using disconnect_future = cl::future<cl::thread_pool, void>;
        using connect_future = cl::future<cl::thread_pool, cl::ftuple<disconnect_future>>;

        template<typename Response>
        using request_future = cl::future<cl::thread_pool, cl::ftuple<request_result, std::unique_ptr<Response>>>;

        explicit client_endpoint(cl::thread_pool *thread_pool,
                                 std::shared_ptr<iauthenticator> authenticator,
                                 std::shared_ptr<iclient_pipe_env> pipe_env,
                                 std::shared_ptr<LocalService> gservice,
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

    template<typename RemoteStub, typename LocalService>
    class client_endpoint<RemoteStub, LocalService>::impl
        : public std::enable_shared_from_this<impl>
    {
    public:
        explicit impl(cl::thread_pool *thread_pool,
                      std::shared_ptr<iauthenticator> authenticator,
                      std::shared_ptr<iclient_pipe_env> pipe_env,
                      std::shared_ptr<LocalService> gservice,
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
        std::shared_ptr<LocalService> m_gservice;

        internal::client_connector m_client_connector;

        mutable std::mutex m_mtx;
        std::unique_ptr<internal::endpoint<RemoteStub>> m_endpoint;
    };

    template<typename RemoteStub, typename LocalService>
    client_endpoint<RemoteStub, LocalService>::impl::impl(cl::thread_pool *thread_pool,
                                                                    std::shared_ptr<iauthenticator> authenticator,
                                                                    std::shared_ptr<iclient_pipe_env> pipe_env,
                                                                    std::shared_ptr<LocalService> gservice,
                                                                    const config &config)
        : m_thread_pool(thread_pool)
        , m_gservice(std::move(gservice))
        , m_client_connector(m_thread_pool, authenticator, pipe_env, config)
    {}

    template<typename RemoteStub, typename LocalService>
    template<typename Request, typename Response, typename StubMethod>
    typename client_endpoint<RemoteStub, LocalService>::template request_future<Response>
        client_endpoint<RemoteStub, LocalService>::impl::make_request(StubMethod stub_method,
                                                                                const Request &req)
    {
        std::lock_guard guard(m_mtx);
        if(!m_endpoint || !m_endpoint->is_connected()) {
            return cl::future(m_thread_pool, cl::ftuple(request_result::no_connection, std::unique_ptr<Response>{}));
        }

        return m_endpoint->template make_request<Request, Response, StubMethod>(stub_method, req);
    }

    template<typename RemoteStub, typename LocalService>
    auto client_endpoint<RemoteStub, LocalService>::impl::connect_async(std::chrono::milliseconds timeout)
    {
        auto f = m_client_connector.create_connection_async(
                                 std::make_shared<internal::service<LocalService>>(m_gservice, m_thread_pool, 0),
                                 timeout);

        return f.then([self = this->weak_from_this()](auto connection) mutable {
            std::shared_ptr s(self);
            return cl::ftuple(s->establish_endpoint(std::move(*connection.lock())));
        });
    }

    template<typename RemoteStub, typename LocalService>
    bool client_endpoint<RemoteStub, LocalService>::impl::is_connected() const
    {
        std::lock_guard guard(m_mtx);
        return m_endpoint && m_endpoint->is_connected();
    }

    template<typename RemoteStub, typename LocalService>
    void client_endpoint<RemoteStub, LocalService>::impl::disconnect()
    {
        std::lock_guard guard(m_mtx);
        if(m_endpoint) {
            m_endpoint->disconnect();
            m_endpoint.reset();
        }
    }

    template<typename RemoteStub, typename LocalService>
    auto client_endpoint<RemoteStub, LocalService>::impl::establish_endpoint(
        std::unique_ptr<internal::iconnection> &&c)
    {
        cl::promise<cl::thread_pool, void> disconnect_promise(m_thread_pool);
        auto disconnect_future = disconnect_promise.get_future();

        std::lock_guard guard(m_mtx);
        m_endpoint = std::make_unique<internal::endpoint<RemoteStub>>(std::move(c), m_thread_pool);
        m_endpoint->set_disconnect_callback(
            cl::thread_pool_task(
                m_thread_pool,
                [disconnect_promise = std::move(disconnect_promise)]() mutable {
                    disconnect_promise.set_value();
                }));

        m_endpoint->start();

        return disconnect_future;
    }

    template<typename RemoteStub, typename LocalService>
    client_endpoint<RemoteStub, LocalService>::client_endpoint(cl::thread_pool *thread_pool,
                                                                         std::shared_ptr<iauthenticator> authenticator,
                                                                         std::shared_ptr<iclient_pipe_env> pipe_env,
                                                                         std::shared_ptr<LocalService> gservice,
                                                                         const config &config)
        : m_impl(std::make_shared<impl>(thread_pool, authenticator, pipe_env, gservice, config))
    {}

    template<typename RemoteStub, typename LocalService>
    auto client_endpoint<RemoteStub, LocalService>::connect_async(std::chrono::milliseconds timeout)
    {
        return m_impl->connect_async(timeout);
    }

    template<typename RemoteStub, typename LocalService>
    bool client_endpoint<RemoteStub, LocalService>::is_connected() const
    {
        return m_impl->is_connected();
    }

    template<typename RemoteStub, typename LocalService>
    void client_endpoint<RemoteStub, LocalService>::disconnect()
    {
        m_impl->disconnect();
    }

    template<typename RemoteStub, typename LocalService>
    template<typename Request, typename Response, typename StubMethod>
    typename client_endpoint<RemoteStub, LocalService>::template request_future<Response>
        client_endpoint<RemoteStub, LocalService>::make_request(StubMethod stub_method,
                                                                          const Request &req)
    {
        return m_impl->template make_request<Request, Response, StubMethod>(stub_method, req);
    }
}
