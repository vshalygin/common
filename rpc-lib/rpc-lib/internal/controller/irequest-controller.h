#pragma once
#include <rpc-lib/types/request-result.h>

namespace google::protobuf {
    class RpcController;
}

namespace vshalygin::rpc {
    class irequest_controller
    {
    public:
        virtual ~irequest_controller() = default;

        virtual void set_result(request_result r) = 0;
    };

    irequest_controller *to_request_controller(google::protobuf::RpcController *);
}
