#include "rpc-client.h"
#include <rpc-lib/client/client-transport/iclient-connection.h>
#include <rpc-lib/client/client-closure/client-closure.h>
#include <rpc-lib/client/server-listener/iserver-listener.h>

#include <common-lib/utils/event/event.h>

#include <google/protobuf/service.h>

namespace vsh::example {
    rpc_client::rpc_client(std::unique_ptr<RpcChannel> channel,
                           std::unique_ptr<rpc::iserver_listener> server_listener,
                           std::shared_ptr<rpc::iclient_connection> connection)
        : service_stub_(channel.release(), ::google::protobuf::Service::STUB_OWNS_CHANNEL)
        , server_listener_(std::move(server_listener))
        , connection_(std::move(connection))
    {}

    int rpc_client::connect()
    {
        server_listener_->start();
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

        auto done = rpc::client_closure::create(std::move(callback));

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
