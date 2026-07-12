#include "irequest-controller.h"
#include "iresponse-controller.h"
#include "request-controller-base.h"
#include "response-controller-base.h"

namespace vshalygin::rpc {
    irequest_controller *to_request_controller(google::protobuf::RpcController *source)
    {
        assert(dynamic_cast<request_controller_base *>(source) != nullptr);
        return static_cast<irequest_controller *>(static_cast<request_controller_base *>(source));
    }

    iresponse_controller *to_response_controller(google::protobuf::RpcController *source)
    {
        assert(dynamic_cast<response_controller_base *>(source) != nullptr);
        return static_cast<iresponse_controller *>(static_cast<response_controller_base *>(source));
    }
}
