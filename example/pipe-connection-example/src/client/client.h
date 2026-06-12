#pragma once
#include <rpc-lib/client-endpoint/client-endpoint.h>
#include <common-lib/pipe/pipe-env.h>

#pragma warning(push, 0)
#include "proto/service.pb.h"
#pragma warning(pop)

namespace vshalygin::example {
    class client
    {
    public:
        client(std::shared_ptr<cl::thread_pool> thread_pool,
               std::shared_ptr<cl::pipe_env> pipe_env,
               const std::string &pipe_listener_name);

        std::unique_ptr<proto::client_response> ask_data(const proto::client_request &req);

    private:
        std::unique_ptr<proto::server_service_Stub> m_stub;
        rpc::client_endpoint m_client_endpoint;
    };
}
