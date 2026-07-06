#pragma once
#include <memory>

namespace vshalygin::rpc {
    class ipipe_endpoint;

    class iclient_pipe_env
    {
    public:
        virtual ~iclient_pipe_env() = default;

        virtual std::shared_ptr<ipipe_endpoint> open_pipe() = 0;
    };
}
