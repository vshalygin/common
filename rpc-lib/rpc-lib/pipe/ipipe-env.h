#pragma once
#include <string>
#include <memory>

namespace vshalygin::rpc {
    class ipipe;

    class ipipe_env
    {
    public:
        virtual ~ipipe_env() = default;

        [[nodiscard]] virtual std::shared_ptr<ipipe> create_pipe() = 0;
        [[nodiscard]] virtual std::shared_ptr<ipipe> open_pipe() = 0;
    };
}
