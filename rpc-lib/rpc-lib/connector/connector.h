#pragma once
#include "rpc-lib/connector/iconnector.h"
#include <string>

namespace vshalygin::rpc {
    class ipipe_env;
    class itransport;
    class iauthenticator;

    class connector
        : public iconnector
    {
    public:
        explicit connector(std::shared_ptr<ipipe_env> pipe_env,
                           std::shared_ptr<iauthenticator> authenticator);

        connector(connector &) = delete;
        connector &operator=(connector &) = delete;

        std::unique_ptr<itransport> create_transport() override;

    private:
        std::shared_ptr<ipipe_env> pipe_env_;
        std::shared_ptr<iauthenticator> authenticator_;
    };
}
