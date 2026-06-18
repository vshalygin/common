#pragma once
#include "iconnector.h"
#include <mutex>

namespace vshalygin::rpc {
    class iauthenticator;
    class ipipe_env;
    class ipipe;

    class client_connector
        : public iconnector
    {
    public:
        explicit client_connector(std::shared_ptr<iauthenticator> authenticator,
                                  std::shared_ptr<ipipe_env> pipe_env);

        client_connector(client_connector &) = delete;
        client_connector &operator=(client_connector &) = delete;

        std::unique_ptr<transport>
            create_transport(std::function<void()> &&stop_callback) const override;

        void interrupt() override;

    private:
        std::shared_ptr<iauthenticator> m_authenticator;
        std::shared_ptr<ipipe_env> m_pipe_env;

        mutable std::mutex m_mtx;
        mutable std::shared_ptr<ipipe> m_curr_pipe;
    };
}
