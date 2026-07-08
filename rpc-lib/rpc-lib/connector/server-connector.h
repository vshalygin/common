#pragma once
#include <memory>
#include <functional>
#include <chrono>

namespace vshalygin::cl {
    class thread_pool;
}

namespace vshalygin::rpc {
    class iauthenticator;
    class iserver_pipe_env;
    class iservice;
    class iconnection;

    enum class server_connector_state
    {
        started,
        stopped
    };

    class server_connector
    {
    public:
        server_connector(std::shared_ptr<cl::thread_pool> thread_pool,
                         std::shared_ptr<iauthenticator> authenticator,
                         std::shared_ptr<iserver_pipe_env> pipe_env,
                         std::function<std::unique_ptr<iservice>(uint64_t)> &&create_service,
                         std::function<void(uint64_t, std::unique_ptr<iconnection>)> &&on_new_connection,
                         std::function<void(server_connector_state)> on_change_state,
                         std::chrono::milliseconds handshake_timeout,
                         std::chrono::milliseconds send_timeout,
                         std::chrono::milliseconds recv_timeout);

        server_connector(const server_connector &) = delete;
        server_connector &operator=(const server_connector &) = delete;

        ~server_connector();

        void start();
        void stop();
        bool is_active() const;

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}
