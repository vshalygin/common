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
    class iconnection;

    class client_connector
    {
    public:
        explicit client_connector(std::shared_ptr<cl::thread_pool> thread_pool,
                                  std::shared_ptr<iauthenticator> authenticator,
                                  std::shared_ptr<iclient_pipe_env> pipe_env,
                                  std::shared_ptr<iservice> service,
                                  std::chrono::milliseconds handshake_timeout,
                                  std::chrono::milliseconds send_timeout,
                                  std::chrono::milliseconds recv_timeout);

        client_connector(client_connector &) = delete;
        client_connector &operator=(client_connector &) = delete;

        ~client_connector();

        future<std::unique_ptr<iconnection>>
            create_connection_async(std::chrono::milliseconds pipe_waiting_timeout);

    private:
        class impl;
        std::shared_ptr<impl> m_impl;
    };
}
