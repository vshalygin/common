#include "request-controller-base.h"
#include "response-controller-base.h"

namespace vshalygin::rpc {
    iresponse_controller *to_response_controller(google::protobuf::RpcController *source)
    {
        assert(dynamic_cast<internal::response_controller_base *>(source) != nullptr);
        return static_cast<iresponse_controller *>(static_cast<internal::response_controller_base *>(source));
    }
}

namespace vshalygin::rpc::internal {
    irequest_controller *to_request_controller(google::protobuf::RpcController *source)
    {
        assert(dynamic_cast<request_controller_base *>(source) != nullptr);
        return static_cast<irequest_controller *>(static_cast<request_controller_base *>(source));
    }
}
