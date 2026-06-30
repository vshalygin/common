#pragma once
#include "ordered-lock.h"

namespace vshalygin::cl::internal {
    template<typename...OrderedLockable>
    class ordered_lock;

    template<typename...Lockables, typename...AddLockables>
    ordered_lock<Lockables..., AddLockables...> push_back
                                     (ordered_lock<Lockables...> &&lock,
                                      AddLockables&...add_locables);
}
