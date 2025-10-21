#include "rpc-server.h"
#include "rpc-server/rpc-service/irpc-service.h"

namespace vsh::example {
    rpc_server::rpc_server(std::unique_ptr<irpc_service> service)
        : service_(std::move(service))
    {}

    rpc_server::~rpc_server() = default;

    void rpc_server::run()
    {
        service_->run();
    }
}
