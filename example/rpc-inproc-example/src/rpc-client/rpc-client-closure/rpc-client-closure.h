#pragma once
#pragma warning(push, 0)
#include <google/protobuf/stubs/callback.h>
#include <google/protobuf/message.h>
#pragma warning(pop)

#include <memory>
#include <functional>

namespace vsh::example {
    class rpc_client_closure final
        : public ::google::protobuf::Closure
    {
        using Closure = ::google::protobuf::Closure;
        using Message = ::google::protobuf::Message;

        rpc_client_closure(std::function<void()> &&on_success);

    public:
        static Closure *create(std::function<void()> &&on_success);

        rpc_client_closure(rpc_client_closure &) = delete;
        rpc_client_closure &operator=(rpc_client_closure &) = delete;

        void Run() override;

    private:
        std::function<void()> on_success_;
    };
}
