#pragma once
#include "rpc-lib/connector/iconnector.h"
#include <string>

namespace vshalygin::rpc {
    class ipipe_env;
    class itransport;

    class connector
        : public iconnector
    {
    public:
        explicit connector(std::shared_ptr<ipipe_env> pipe_env,
                           const std::string &listener_pipe_name);

        connector(connector &) = delete;
        connector &operator=(connector &) = delete;

        std::unique_ptr<itransport> create_transport() override;

    private:
        std::shared_ptr<ipipe_env> pipe_env_;
        const std::string listener_pipe_name_;
    };
}
