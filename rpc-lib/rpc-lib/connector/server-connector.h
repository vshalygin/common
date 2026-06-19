#pragma once
#include "iconnector.h"
#include <mutex>

namespace vshalygin::cl {
    class thread_pool;
}

namespace vshalygin::rpc {
    class iauthenticator;
    class ipipe_env;
    class ipipe_endpoint;

    class server_connector
        : public iconnector
    {
    public:
        explicit server_connector(std::shared_ptr<cl::thread_pool> thread_pool,
                                  std::shared_ptr<iauthenticator> authenticator,
                                  std::shared_ptr<ipipe_env> pipe_env);

        server_connector(server_connector &) = delete;
        server_connector &operator=(server_connector &) = delete;

        std::unique_ptr<transport>
            create_transport(std::function<void()> &&stop_callback) const override;

        void interrupt() override;

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;
        std::shared_ptr<iauthenticator> m_authenticator;
        std::shared_ptr<ipipe_env> m_pipe_env;

        mutable std::mutex m_mtx;
        mutable std::shared_ptr<ipipe_endpoint> m_curr_pipe_endpoint;
    };
}
