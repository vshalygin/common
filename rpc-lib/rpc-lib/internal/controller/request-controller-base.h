#pragma once
#include "null-rpc-controller.h"
#include "irequest-controller.h"

namespace vshalygin::rpc::internal {
    class request_controller_base
        : public null_rpc_controller
        , public irequest_controller
    {};
}
