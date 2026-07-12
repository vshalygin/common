#pragma once
#include "null-rpc-controller.h"
#include "irequest-controller.h"

namespace vshalygin::rpc {
    class request_controller_base
        : public null_rpc_controller
        , public irequest_controller
    {};
}
