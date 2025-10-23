#include "client-closure.h"

namespace vsh::rpc {
    client_closure::client_closure(std::function<void()> &&on_success)
        : on_success_(std::move(on_success))
    {}

    ::google::protobuf::Closure *client_closure::create(std::function<void()> &&on_success)
    {
        return new client_closure(std::move(on_success));
    }

    void client_closure::Run()
    {
        on_success_();

        delete this;
    }
}
