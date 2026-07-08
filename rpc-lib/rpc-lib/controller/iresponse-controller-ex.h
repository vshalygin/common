#pragma once
#include "iresponse-controller.h"
#include <rpc-lib/types/response-result.h>

namespace vshalygin::rpc {
    class iresponse_controller_ex
        : public iresponse_controller
    {
    public:
        virtual void set_response_result(response_result) = 0;
    };

    iresponse_controller_ex *to_response_controller_ex(google::protobuf::RpcController *);
}
