#pragma once
#include "irpc-server.h"

#include <memory>

namespace vsh::example {
    class irpc_service;

    class rpc_server
        : public irpc_server
    {
    public:
        explicit rpc_server(std::unique_ptr<irpc_service> service);
        ~rpc_server() override;

        rpc_server(rpc_server &) = delete;
        rpc_server &operator=(rpc_server &) = delete;

        void run() override;

    private:
        std::unique_ptr<irpc_service> service_;
    };
}
