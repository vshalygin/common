#pragma once
#include "ordered-lock-locked.h"

namespace vshalygin::cl::internal {
    template<typename...OrderedLockable>
    ordered_lock<true, OrderedLockable...>::ordered_lock(OrderedLockable&...lockables)
        : base_type(lockables)
    {
        lock();
    }

    template<typename...OrderedLockable>
    ordered_lock<true, OrderedLockable...>::ordered_lock
                             (ordered_lock<true, OrderedLockable...> &&other)
        : base_type(other.get_ordered_ptr_tuple())
    {
        other.clear();
    }

    template<typename...OrderedLockable>
    ordered_lock<true, OrderedLockable...> &ordered_lock<true, OrderedLockable...>::operator=
                             (ordered_lock<true, OrderedLockable...> &&other)
    {
        if(this != &other) {
            if(is_valid()) {
                unlock();
            }
            set_ordered_ptr_tuple(other.get_ordered_ptr_tuple());
            other.clear();
        }

        return *this;
    }

    template<typename...OrderedLockable>
    ordered_lock<true, OrderedLockable...>::ordered_lock
                             (ordered_lock<false, OrderedLockable...> &&other)
        : base_type(other.get_ordered_ptr_tuple())
    {
        lock();
    }

    template<typename...OrderedLockable>
    ordered_lock<true, OrderedLockable...> &ordered_lock<true, OrderedLockable...>::operator=
                             (ordered_lock<false, OrderedLockable...> &&other)
    {
        if(is_valid()) {
            unlock();
        }
        set_ordered_ptr_tuple(other.get_ordered_ptr_tuple());
        lock();
        other.clear();

        return *this;
    }
}
