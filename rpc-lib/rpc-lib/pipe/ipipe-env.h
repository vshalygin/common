#pragma once
#include <string>
#include <memory>

namespace vshalygin::rpc {
    class ipipe_endpoint;

    class ipipe_env
    {
    public:
        virtual ~ipipe_env() = default;

        [[nodiscard]] virtual std::shared_ptr<ipipe_endpoint> create_pipe() = 0;
        [[nodiscard]] virtual std::shared_ptr<ipipe_endpoint> open_pipe() = 0;
    };
}
