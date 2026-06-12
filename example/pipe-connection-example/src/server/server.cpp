#include "server.h"
#include "server-service.h"
#include "rpc-lib/transport/in-process/pipe-listener.h"

#pragma warning(push, 0)
#include "proto/service.pb.h"
#pragma warning(pop)

namespace vshalygin::example {
    server::server(std::shared_ptr<cl::thread_pool> thread_pool,
                   std::shared_ptr<cl::pipe_env> pipe_env,
                   const std::string &pipe_listener_name)
    {
        auto listener = std::make_unique<rpc::pipe_listener>(pipe_env, pipe_listener_name);
        auto service = std::make_shared<rpc::service<server_service>>(std::make_unique<server_service>());
        server_endpoint_ = rpc::server_endpoint(std::move(listener), std::move(service), thread_pool);

        server_endpoint_.start_listen();
    }
}