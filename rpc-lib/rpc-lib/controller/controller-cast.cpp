#include "irequest-controller.h"
#include "request-controller-base.h"

namespace vshalygin::rpc {
    irequest_controller *to_request_controller(google::protobuf::RpcController *source)
    {
        assert(dynamic_cast<request_controller_base *>(source) != nullptr);
        return static_cast<irequest_controller *>(static_cast<request_controller_base *>(source));
    }
}
