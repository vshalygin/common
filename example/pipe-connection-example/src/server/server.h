#pragma once
#include "rpc-lib/server-endpoint/server-endpoint.h"
#include "rpc-lib/service/service.h"
#include "common-lib/pipe/pipe-env.h"

namespace vshalygin::example {
    class server final
    {
    public:
        explicit server(std::shared_ptr<cl::thread_pool> thread_pool,
                        std::shared_ptr<cl::pipe_env> pipe_env,
                        const std::string &pipe_listener_name);

        server(server &) = delete;
        server &operator=(server &) = delete;

        server(server &&) = default;
        server &operator=(server &&) = default;

    private:
        rpc::server_endpoint server_endpoint_;
    };
}
