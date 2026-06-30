#pragma once
#include "internal/ordered-lock/ordered-lock-all.h"

namespace vshalygin::cl {
    using defer_lock_t = internal::defer_lock_t;
    using adopt_lock_t = internal::adopt_lock_t;

    template<typename...OrderedLockable>
    using ordered_lock = internal::ordered_lock<OrderedLockable...>;

    template<typename...Lockables, typename...AddLockables>
    ordered_lock<Lockables..., AddLockables...> push_back
                                    (ordered_lock<Lockables...> &&lock,
                                     AddLockables&...add_locables)
    {
        return internal::push_back(std::move(lock), add_locables...);
    }
}
