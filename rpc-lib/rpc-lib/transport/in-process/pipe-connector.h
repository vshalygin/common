#pragma once
#include "rpc-lib/interface/iconnector.h"
#include <string>

namespace vshalygin::rpc {
    class ipipe_env;

    class pipe_connector
        : public iconnector
    {
    public:
        explicit pipe_connector(std::shared_ptr<ipipe_env> pipe_env,
                                const std::string &listener_pipe_name);

        pipe_connector(pipe_connector &) = delete;
        pipe_connector &operator=(pipe_connector &) = delete;

        std::unique_ptr<itransport> create_transport() override;

    private:
        std::shared_ptr<ipipe_env> pipe_env_;
        const std::string listener_pipe_name_;
    };
}
