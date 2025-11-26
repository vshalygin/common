#pragma once
#include "irpc-server.h"

#pragma warning(push, 0)
#include "proto/service.pb.h"
#pragma warning(pop)

#include <memory>

namespace vsh::rpc {
    class iserver_transport;
}

namespace vsh::example {
    class rpc_server
        : public irpc_server
    {
    public:
        explicit rpc_server(std::unique_ptr<rpc::iserver_transport> transport,
                            std::unique_ptr<proto::Service> service);
        ~rpc_server() override;

        rpc_server(rpc_server &) = delete;
        rpc_server &operator=(rpc_server &) = delete;

        void run() override;

    private:
        std::unique_ptr<rpc::iserver_transport> m_transport;
        std::unique_ptr<proto::Service> m_service;
    };
}
