#pragma once
#include "ordered-lock-functions.h"

namespace vshalygin::cl::internal {
    template<typename...Lockables, typename...AddLockables>
    ordered_lock<Lockables..., AddLockables...> push_back
                                     (ordered_lock<Lockables...> &&lock,
                                      AddLockables&...add_locables)
    {
        return lock.push_back(add_locables...);
    }
}
