#include "rpc-client.h"
#include "rpc-client/rpc-client-transport/irpc-client-connection.h"

#include "rpc-client/rpc-client-closure/rpc-client-closure.h"

#include <common-lib/utils/event/event.h>

#include <google/protobuf/service.h>

namespace vsh::example {
    rpc_client::rpc_client(std::unique_ptr<RpcChannel> channel,
                           std::shared_ptr<irpc_client_connection> connection)
        : service_stub_(channel.release(), ::google::protobuf::Service::STUB_OWNS_CHANNEL)
        , connection_(std::move(connection))
    {}

    int rpc_client::connect()
    {
        return connection_->connect();
    }

    int rpc_client::disconnect()
    {
        return connection_->close();
    }

    template<typename Request, typename Response, auto Method>
    std::shared_ptr<Response> rpc_client::call_method(const Request &req)
    {
        auto response = std::make_shared<Response>();
        auto response_ptr = response.get();

        common_lib::event sync_event;
        auto callback = [&sync_event]() {
            sync_event.set();
        };

        auto done = rpc_client_closure::create(std::move(callback));

        (service_stub_.*Method)(nullptr, //TODO add rpc_controller
                                &req,
                                response_ptr,
                                done);

        sync_event.wait();

        return response;
    }

    std::shared_ptr<proto::GetUserResponse> rpc_client::get_user(const proto::GetUserRequest &req)
    {
        return call_method<proto::GetUserRequest, proto::GetUserResponse, &proto::Service_Stub::GetUser>(req);
    }

    std::shared_ptr<proto::GetUserResponse> rpc_client::get_user2(const proto::GetUserRequest &req)
    {
        return call_method<proto::GetUserRequest, proto::GetUserResponse, &proto::Service_Stub::GetUser2>(req);
    }
}
