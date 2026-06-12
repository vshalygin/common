#include "client.h"
#include "rpc-lib/channel/channel.h"
#include "rpc-lib/connection/connection.h"
#include "rpc-lib/transport/in-process/pipe-connector.h"

namespace vshalygin::example {
    client::client(std::shared_ptr<cl::thread_pool> thread_pool,
                   std::shared_ptr<cl::pipe_env> pipe_env,
                   const std::string &pipe_listener_name)
    {
        auto channel = std::make_unique<rpc::channel>();
        auto connection = std::make_shared<rpc::connection>(thread_pool);
        auto connector = std::make_unique<rpc::pipe_connector>(pipe_env, pipe_listener_name);
        m_stub = std::make_unique<proto::server_service_Stub>(channel.get());
        m_client_endpoint = rpc::client_endpoint(nullptr, std::move(channel), connection, std::move(connector));
        m_client_endpoint.connect();
    }

    std::unique_ptr<proto::client_response> client::ask_data(const proto::client_request &req)
    {
        using Request = proto::client_request;
        using Response = proto::client_response;

        return m_client_endpoint
            .make_request<Request, Response>(req, *m_stub, &proto::server_service_Stub::get_data);
    }
}