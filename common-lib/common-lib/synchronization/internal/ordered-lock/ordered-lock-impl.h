#pragma once
#include "ordered-lock.h"
#include <common-lib/utils/tuple-utils.h>
#include <common-lib/utils/do-on-destruct.h>

namespace vshalygin::cl::internal {
    template<typename...OrderedLockable>
    ordered_lock<OrderedLockable...>::ordered_lock(OrderedLockable&...lockables)
        : m_ordered_ptr_tuple(sort_tuple2<order_comparator>(init_ptr_tuple{ &lockables... }))
    {}

    template<typename...OrderedLockable>
    ordered_lock<OrderedLockable...>::~ordered_lock()
    {
        if(is_locked()) {
            unlock();
        }
    }

    template<typename...OrderedLockable>
    ordered_lock<OrderedLockable...>::ordered_lock
                            (ordered_lock<OrderedLockable...> &&other)
        : m_is_locked(other.m_is_locked)
        , m_ordered_ptr_tuple(other.m_ordered_ptr_tuple)
    {
        other.clear();
        other.m_is_locked = false;
    }

    template<typename...OrderedLockable>
    ordered_lock<OrderedLockable...> &ordered_lock<OrderedLockable...>::operator=
                                             (ordered_lock<OrderedLockable...> &&other)
    {
        if(this != &other) {
            if(is_locked()) {
                unlock();
            }
            m_ordered_ptr_tuple = other.m_ordered_ptr_tuple;

            other.clear();
            other.m_is_locked = false;
        }

        return *this;
    }

    template<typename...OrderedLockable>
    void ordered_lock<OrderedLockable...>::lock()
    {
        do_safe_lock(std::make_index_sequence<tuple_size_v<ordered_ptr_tuple>>());
    }

    template<typename...OrderedLockable>
    template<size_t...I>
    void ordered_lock<OrderedLockable...>::do_safe_lock(std::index_sequence<I...>)
    {
        ordered_ptr_tuple locked;
        do_on_destruct d([&locked]() {
            for_each_tuple_element_reverse(locked, [](auto el_ptr) {
                if(el_ptr) {
                    el_ptr->unlock();
                }
            });
        });

        ((std::get<I>(m_ordered_ptr_tuple)->lock(),
          std::get<I>(locked) = std::get<I>(m_ordered_ptr_tuple)), ...);

        d.release();
    }

    template<typename...OrderedLockable>
    void ordered_lock<OrderedLockable...>::unlock() noexcept
    {
        for_each_tuple_element_reverse(m_ordered_ptr_tuple, [](auto el_ptr) {
            el_ptr->unlock();
        });
    }

    template<typename...OrderedLockable>
    bool ordered_lock<OrderedLockable...>::is_locked() const noexcept
    {
        return m_is_locked;
    }

    template<typename...OrderedLockable>
    ordered_lock<OrderedLockable...>::operator bool() const noexcept
    {
        return std::get<0>(m_ordered_ptr_tuple);
    }

    template<typename...OrderedLockable>
    void ordered_lock<OrderedLockable...>::clear() noexcept
    {
        for_each_tuple_element(m_ordered_ptr_tuple, [](auto &el_ptr) {
            el_ptr = nullptr;
        });
    }
}
