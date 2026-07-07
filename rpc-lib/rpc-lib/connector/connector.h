#pragma once
#include <rpc-lib/types/future.h>
#include <chrono>

namespace vshalygin::cl {
    class thread_pool;
}

namespace vshalygin::rpc {
    class iauthenticator;
    class iclient_pipe_env;
    class iservice;
    class connection;

    class connector
    {
    public:
        explicit connector(std::shared_ptr<cl::thread_pool> thread_pool,
                           std::shared_ptr<iauthenticator> authenticator,
                           std::shared_ptr<iclient_pipe_env> pipe_env,
                           std::shared_ptr<iservice> service,
                           std::chrono::milliseconds send_timeout,
                           std::chrono::milliseconds recv_timeout);

        connector(connector &) = delete;
        connector &operator=(connector &) = delete;

        ~connector();

        future<connection> create_connection_async(std::chrono::milliseconds handshake_timeout);
        void cancel_connect_waiting();

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;
        std::shared_ptr<iauthenticator> m_authenticator;
        std::shared_ptr<iclient_pipe_env> m_pipe_env;
        std::shared_ptr<iservice> m_service;
        const std::chrono::milliseconds m_send_timeout;
        const std::chrono::milliseconds m_recv_timeout;
    };
}
