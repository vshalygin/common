#pragma once
#include "order-lock-base.h"

namespace vshalygin::cl::internal {
    template<typename...OrderedLockable>
    ordered_lock_base<OrderedLockable...>::ordered_lock_base(OrderedLockable&...lockables)
        : m_ordered_ptr_tuple(sort_tuple2<order_comparator>(init_ptr_tuple{ &lockables... }))
    {}

    template<typename...OrderedLockable>
    ordered_lock_base<OrderedLockable...>::ordered_lock_base(const ordered_ptr_tuple &tuple)
        : m_ordered_ptr_tuple(tuple)
    {}

    template<typename...OrderedLockable>
    bool ordered_lock_base<OrderedLockable...>::is_valid() const noexcept
    {
        if constexpr(tuple_size_v<ordered_ptr_tuple> == 0) {
            return false;
        } else {
            return std::get<0>(m_ordered_ptr_tuple);
        }
    }

    template<typename...OrderedLockable>
    void ordered_lock_base<OrderedLockable...>::lock()
    {
        for_each_tuple_element(m_ordered_ptr_tuple, [](auto el_ptr) {
            el_ptr->lock();
        });
    }

    template<typename...OrderedLockable>
    void ordered_lock_base<OrderedLockable...>::unlock()
    {
        //TODO! нужно разблокировать в обратном порядке!
        for_each_tuple_element(m_ordered_ptr_tuple, [](auto el_ptr) {
            el_ptr->unlock();
        });
    }

    template<typename...OrderedLockable>
    void ordered_lock_base<OrderedLockable...>::clear() noexcept
    {
        for_each_tuple_element(m_ordered_ptr_tuple, [](auto &el_ptr) {
            el_ptr = nullptr;
        });
    }
}
