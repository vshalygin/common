#pragma once
#include "ordered-lock.h"

namespace vshalygin::cl::internal {
    template<typename...OrderedLockable>
    class ordered_lock<false, OrderedLockable...>
        : private ordered_lock_base<OrderedLockable...>
    {
        using base_type = ordered_lock_base<OrderedLockable...>;

    public:
        ordered_lock() = default;
        explicit ordered_lock(OrderedLockable&...lockables);

        ordered_lock(ordered_lock<false, OrderedLockable...> &&other);
        ordered_lock &operator=(ordered_lock<false, OrderedLockable...> &&other);

        ordered_lock(ordered_lock<true, OrderedLockable...> &&other);
        ordered_lock &operator=(ordered_lock<true, OrderedLockable...> &&other);

        ordered_lock(const ordered_lock &) = delete;
        ordered_lock &operator=(const ordered_lock &) = delete;
    };
}
