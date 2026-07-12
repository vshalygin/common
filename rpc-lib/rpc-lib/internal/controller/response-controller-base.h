#pragma once
#include "null-rpc-controller.h"
#include <rpc-lib/iresponse-controller.h>

namespace vshalygin::rpc::internal {
    class response_controller_base
        : public null_rpc_controller
        , public iresponse_controller
    {};
}
