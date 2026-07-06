#pragma once
#include <rpc-lib/types/future.h>
#include <chrono>

namespace vshalygin::cl {
    class thread_pool;
}

namespace vshalygin::rpc {
    class iauthenticator;
    class ipipe_env;
    class iservice;

    class client_connector
    {
    public:
        explicit client_connector(std::shared_ptr<cl::thread_pool> thread_pool,
                                  std::shared_ptr<iauthenticator> authenticator,
                                  std::shared_ptr<ipipe_env> pipe_env,
                                  std::shared_ptr<iservice> service,
                                  std::chrono::milliseconds req_timeout,
                                  std::chrono::milliseconds res_timeout);

        client_connector(client_connector &) = delete;
        client_connector &operator=(client_connector &) = delete;

        future<connection> create_connection_async(std::chrono::milliseconds timeout);

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;
        std::shared_ptr<iauthenticator> m_authenticator;
        std::shared_ptr<ipipe_env> m_pipe_env;
        std::shared_ptr<iservice> m_service;
        const std::chrono::milliseconds m_req_timeout;
        const std::chrono::milliseconds m_res_timeout;
    };
}
