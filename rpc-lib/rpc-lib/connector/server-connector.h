#pragma once
#include <rpc-lib/types/future.h>
#include <mutex>

namespace vshalygin::cl {
    class thread_pool;
}

namespace vshalygin::rpc {
    class iauthenticator;
    class ipipe_env;
    class ipipe_endpoint;
    class iservice;
    class connection;

    class server_connector
    {
    public:
        explicit server_connector(std::shared_ptr<cl::thread_pool> thread_pool,
                                  std::shared_ptr<iauthenticator> authenticator,
                                  std::shared_ptr<ipipe_env> pipe_env,
                                  std::shared_ptr<iservice> service,
                                  std::chrono::milliseconds req_timeout,
                                  std::chrono::milliseconds res_timeout);

        server_connector(server_connector &) = delete;
        server_connector &operator=(server_connector &) = delete;

        future<connection> create_connection_async();

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;
        std::shared_ptr<iauthenticator> m_authenticator;
        std::shared_ptr<ipipe_env> m_pipe_env;
        std::shared_ptr<iservice> m_service;
        const std::chrono::milliseconds m_req_timeout;
        const std::chrono::milliseconds m_res_timeout;
    };
}
