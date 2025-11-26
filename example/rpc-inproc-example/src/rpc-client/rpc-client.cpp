#include "rpc-client.h"
#include "rpc-client/rpc-client-transport/rpc-client-transport.h"

#include <rpc-lib/client/client-closure/client-closure.h>
#include <rpc-lib/common/listener/ilistener.h>

#include <common-lib/utils/event/event.h>

#include <google/protobuf/service.h>

namespace vsh::example {
    rpc_client::rpc_client(std::shared_ptr<cl::ithread_pool> thread_pool)
        : rpc_client(std::move(thread_pool), std::make_shared<rpc_client_transport>())
    {}

    rpc_client::rpc_client(std::shared_ptr<cl::ithread_pool> thread_pool,
                           std::shared_ptr<rpc_client_transport> rpc_client_transport)
        : rpc::client_base(std::move(thread_pool), rpc_client_transport, rpc_client_transport)
        , service_stub_(get_channel())
    {}

    std::unique_ptr<proto::GetUserResponse> rpc_client::get_user(const proto::GetUserRequest &req)
    {
        return call_method<proto::GetUserRequest, proto::GetUserResponse>(req,
                                                                          service_stub_,
                                                                          &proto::Service_Stub::GetUser);
    }

    std::unique_ptr<proto::GetUserResponse> rpc_client::get_user2(const proto::GetUserRequest &req)
    {
        return call_method<proto::GetUserRequest, proto::GetUserResponse>(req,
                                                                          service_stub_,
                                                                          &proto::Service_Stub::GetUser2);
    }
}
