#pragma once
#include <memory>

namespace vshalygin::rpc {
    class ipipe_endpoint;

    class iserver_pipe_env
    {
    public:
        virtual ~iserver_pipe_env() = default;

        virtual std::shared_ptr<ipipe_endpoint> create_pipe() = 0;
    };
}
