#pragma once
#include "ordered-lock-unlocked.h"

namespace vshalygin::cl::internal {
    template<typename...OrderedLockable>
    ordered_lock<false, OrderedLockable...>::ordered_lock(OrderedLockable&...lockables)
        : base_type(lockables)
    {}

    template<typename...OrderedLockable>
    ordered_lock<false, OrderedLockable...>::ordered_lock
                     (ordered_lock<false, OrderedLockable...> &&other)
        : base_type(other.get_ordered_ptr_tuple())
    {
        other.clear();
    }

    template<typename...OrderedLockable>
    ordered_lock<false, OrderedLockable...> &ordered_lock<false, OrderedLockable...>::operator=
                     (ordered_lock<false, OrderedLockable...> &&other)
    {
        if(this != &other) {
            set_ordered_ptr_tuple(other.get_ordered_ptr_tuple());
            other.clear();
        }

        return *this;
    }

    template<typename...OrderedLockable>
    ordered_lock<false, OrderedLockable...>::ordered_lock
                        (ordered_lock<true, OrderedLockable...> &&other)
    {
        if(other.is_valid()) {
            other.unlock();
        }

        set_ordered_ptr_tuple(other.get_ordered_ptr_tuple());
        other.clear();
    }

    template<typename...OrderedLockable>
    ordered_lock<false, OrderedLockable...> &ordered_lock<false, OrderedLockable...>::operator=
                        (ordered_lock<true, OrderedLockable...> &&other)
    {
        if(other.is_valid()) {
            other.unlock();
        }

        set_ordered_ptr_tuple(other.get_ordered_ptr_tuple());
        other.clear();
    }
}
