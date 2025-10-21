#pragma once
#include "irpc-server.h"

#pragma warning(push, 0)
#include "proto/service.pb.h"
#pragma warning(pop)

#include <memory>

namespace vsh::example {
    class irpc_server_transport;

    class rpc_server
        : public irpc_server
    {
    public:
        explicit rpc_server(std::unique_ptr<irpc_server_transport> transport,
                            std::unique_ptr<proto::Service> service);
        ~rpc_server() override;

        rpc_server(rpc_server &) = delete;
        rpc_server &operator=(rpc_server &) = delete;

        void run() override;

    private:
        std::unique_ptr<irpc_server_transport> transport_;
        std::unique_ptr<proto::Service> service_;
    };
}
