#pragma once
#include "null-rpc-controller.h"
#include "iresponse-controller.h"

namespace vshalygin::rpc {
    class response_controller_base
        : public null_rpc_controller
        , public iresponse_controller
    {};
}
