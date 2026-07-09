#pragma once
#include "endpoint.h"
#include <rpc-lib/connector/client-connector.h>
#include <rpc-lib/types/connection-state.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <functional>

namespace vshalygin::cl {
    class thread_pool;
}

namespace vshalygin::rpc {
    class iauthenticator;
    class iclient_pipe_env;

    template<typename GServiceStub>
    class client_endpoint
    {
    public:
        using on_state_change_t = std::function<void(uint64_t, connection_state)>;

        template<typename GService>
        explicit client_endpoint(on_state_change_t &&on_state_change,
                                 std::shared_ptr<cl::thread_pool> thread_pool,
                                 std::shared_ptr<iauthenticator> authenticator,
                                 std::shared_ptr<iclient_pipe_env> pipe_env,
                                 std::shared_ptr<GService> gservice,
                                 std::chrono::milliseconds handshake_timeout,
                                 std::chrono::milliseconds send_timeout,
                                 std::chrono::milliseconds recv_timeout);

        client_endpoint(const client_endpoint &) = delete;
        client_endpoint &operator=(const client_endpoint &) = delete;

        void connect(std::chrono::milliseconds timeout);
        void is_connected();
        void disconnect();


    private:
        std::shared_ptr<on_state_change_t> m_on_state_change;

        client_connector m_client_connector;

        std::mutex m_mtx;
        uint64_t m_next_connection_id = 0;
        std::unique_ptr<endpoint<GServiceStub>> m_endpoint;
    };

    template<typename GServiceStub>
    template<typename GService>
    client_endpoint<GServiceStub>::client_endpoint(on_state_change_t &&on_state_change,
                                                   std::shared_ptr<cl::thread_pool> thread_pool,
                                                   std::shared_ptr<iauthenticator> authenticator,
                                                   std::shared_ptr<iclient_pipe_env> pipe_env,
                                                   std::shared_ptr<GService> gservice,
                                                   std::chrono::milliseconds handshake_timeout,
                                                   std::chrono::milliseconds send_timeout,
                                                   std::chrono::milliseconds recv_timeout)
        : m_on_state_change(std::make_shared<on_state_change_t>(std::move(on_state_change)))
        , m_client_connector(thread_pool, authenticator, pipe_env,
                             )
    {}

    template<typename GServiceStub>
    void client_endpoint<GServiceStub>::connect(std::chrono::milliseconds timeout)
    {
        auto f = m_client_connector.create_connection_async(timeout);

        std::unique_ptr<iconnection> connection;
        f.get().apply([&connection](std::unique_ptr<iconnection> &&c) { connection = std::move(c); });
    }

    template<typename GServiceStub>
    void client_endpoint<GServiceStub>::is_connected()
    {

    }

    template<typename GServiceStub>
    void client_endpoint<GServiceStub>::disconnect()
    {

    }
}
