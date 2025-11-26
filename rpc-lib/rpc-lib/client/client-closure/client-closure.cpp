#include "client-closure.h"

namespace vsh::rpc {
    client_closure::client_closure(std::function<void()> &&on_success)
        : m_on_success(std::move(on_success))
    {}

    ::google::protobuf::Closure *client_closure::create(std::function<void()> &&on_success)
    {
        return new client_closure(std::move(on_success));
    }

    void client_closure::Run()
    {
        m_on_success();

        delete this;
    }
}
