#include "rpc-client.h"
#include "rpc-client/rpc-client-transport/irpc-client-connection.h"


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

    proto::GetUserResponse rpc_client::get_user(const proto::GetUserRequest &req)
    {
        proto::GetUserResponse response;
        service_stub_.GetUser(nullptr, //TODO add rpc_controller and closure
                              &req,
                              &response,
                              nullptr);

        return response;
    }
}
