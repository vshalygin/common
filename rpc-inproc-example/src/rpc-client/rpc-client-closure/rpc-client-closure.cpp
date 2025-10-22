#include "rpc-client-closure.h"

namespace vsh::example {
    rpc_client_closure::rpc_client_closure(std::function<void()> &&on_success)
        : on_success_(std::move(on_success))
    {}

    ::google::protobuf::Closure *rpc_client_closure::create(std::function<void()> &&on_success)
    {
        return new rpc_client_closure(std::move(on_success));
    }

    void rpc_client_closure::Run()
    {
        on_success_();

        delete this;
    }
}
