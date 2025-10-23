#pragma once
#pragma warning(push, 0)
#include <google/protobuf/stubs/callback.h>
#include <google/protobuf/message.h>
#pragma warning(pop)

#include <memory>
#include <functional>

namespace vsh::rpc {
    class client_closure final
        : public ::google::protobuf::Closure
    {
        using Closure = ::google::protobuf::Closure;
        using Message = ::google::protobuf::Message;

        client_closure(std::function<void()> &&on_success);

    public:
        static Closure *create(std::function<void()> &&on_success);

        client_closure(client_closure &) = delete;
        client_closure &operator=(client_closure &) = delete;

        void Run() override;

    private:
        std::function<void()> on_success_;
    };
}
