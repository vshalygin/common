#pragma once
#include "order-lock-base.h"
#include "order-lock-base-impl.h"

namespace vshalygin::cl::internal {
    template<bool IsLocked, typename...OrderedLockable>
    class ordered_lock;
}
