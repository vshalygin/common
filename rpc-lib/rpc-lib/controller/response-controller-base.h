#pragma once
#include "null-rpc-controller.h"
#include "iresponse-controller-ex.h"

namespace vshalygin::rpc {
    class response_controller_base
        : public null_rpc_controller
        , public iresponse_controller_ex
    {};
}
