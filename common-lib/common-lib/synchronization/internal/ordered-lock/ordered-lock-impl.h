#pragma once
#include "ordered-lock.h"
#include "compare-ordered-lockable-tuples.h"

#include <common-lib/utils/do-on-destruct.h>

namespace vshalygin::cl::internal {
    template<typename...OrderedLockable>
    ordered_lock<OrderedLockable...>::ordered_lock() noexcept
        : m_init_ptr_tuple()
        , m_ordered_ptr_ref_tuple(sort_tuple2<order_comparator>(tie_tuple(m_init_ptr_tuple)))
        , m_is_locked(false)
    {}

    template<typename...OrderedLockable>
    ordered_lock<OrderedLockable...>::ordered_lock(OrderedLockable&...lockables)
        : m_init_ptr_tuple(&lockables...)
        , m_ordered_ptr_ref_tuple(sort_tuple2<order_comparator>(tie_tuple(m_init_ptr_tuple)))
        , m_is_locked(false)
    {
        lock();
    }

    template<typename...OrderedLockable>
    ordered_lock<OrderedLockable...>::ordered_lock(defer_lock_t, OrderedLockable&...lockables) noexcept
        : m_init_ptr_tuple(&lockables...)
        , m_ordered_ptr_ref_tuple(sort_tuple2<order_comparator>(tie_tuple(m_init_ptr_tuple)))
        , m_is_locked(false)
    {}

    template<typename...OrderedLockable>
    ordered_lock<OrderedLockable...>::ordered_lock(adopt_lock_t, OrderedLockable&...lockables) noexcept
        : m_init_ptr_tuple(&lockables...)
        , m_ordered_ptr_ref_tuple(sort_tuple2<order_comparator>(tie_tuple(m_init_ptr_tuple)))
        , m_is_locked(true)
    {}

    template<typename...OrderedLockable>
    ordered_lock<OrderedLockable...>::~ordered_lock()
    {
        if(is_locked()) {
            unlock();
        }
    }

    template<typename...OrderedLockable>
    void ordered_lock<OrderedLockable...>::lock()
    {
        do_safe_lock(std::make_index_sequence<tuple_size_v<init_ptr_tuple>>());
        m_is_locked = true;
    }

    template<typename...OrderedLockable>
    template<size_t...I>
    void ordered_lock<OrderedLockable...>::do_safe_lock(std::index_sequence<I...>)
    {
        ordered_ptr_tuple locked;
        do_on_destruct d([&locked]() mutable {
            for_each_tuple_element_reverse(locked, [](auto el_ptr) {
                if(el_ptr) {
                    el_ptr->unlock();
                }
            });
        });

        ((std::get<I>(m_ordered_ptr_ref_tuple)->lock(),
          std::get<I>(locked) = std::get<I>(m_ordered_ptr_ref_tuple)), ...);

        d.release();
    }

    template<typename...OrderedLockable>
    void ordered_lock<OrderedLockable...>::unlock() noexcept
    {
        for_each_tuple_element_reverse(m_ordered_ptr_ref_tuple, [](auto el_ptr) {
            el_ptr->unlock();
        });

        m_is_locked = false;
    }

    template<typename...OrderedLockable>
    bool ordered_lock<OrderedLockable...>::is_locked() const noexcept
    {
        return m_is_locked;
    }

    template<typename...OrderedLockable>
    ordered_lock<OrderedLockable...>::operator bool() const noexcept
    {
        return std::get<0>(m_init_ptr_tuple);
    }

    template<typename...OrderedLockable>
    void ordered_lock<OrderedLockable...>::clear() noexcept
    {
        for_each_tuple_element(m_init_ptr_tuple, [](auto &el_ptr) {
            el_ptr = nullptr;
        });

        m_is_locked = false;
    }

    template<typename...OrderedLockable>
    template<typename...AddLockables>
    ordered_lock<OrderedLockable..., AddLockables...>
        ordered_lock<OrderedLockable...>::push_back(AddLockables&...add_locables)
    {
        static_assert(sizeof...(AddLockables) > 0,
                      "no add lockable presented");
        static_assert(is_ordered_lockable_types_valid_v<AddLockables...>,
                      "ordered lockable type are invalid");
        static_assert(compare_ordered_lockable_tuples_v<std::tuple<OrderedLockable...>,
                                                        std::tuple<AddLockables...>>,
                      "order value of adding lockable must be greater than any presented");

        return push_back_impl<AddLockables...>(add_locables...,
                              std::make_index_sequence<tuple_size_v<init_ptr_tuple>>());
    }

    template<typename...OrderedLockable>
    template<typename...AddLockables, size_t...I>
    ordered_lock<OrderedLockable..., AddLockables...>
        ordered_lock<OrderedLockable...>::push_back_impl(AddLockables&...add_locables,
                                                         std::index_sequence<I...>)
    {
        using ans_t = ordered_lock<OrderedLockable..., AddLockables...>;
        ans_t ans;
        if(is_locked()) {
            auto add_ordered_lock = ordered_lock<AddLockables...>{ add_locables... };
            ans = ans_t( adopt_lock_t{},
                         *std::get<I>(m_init_ptr_tuple)...,
                         add_locables... );
            add_ordered_lock.clear();
        } else {
            ans = ans_t(defer_lock_t{},
                        *std::get<I>(m_init_ptr_tuple)...,
                        add_locables...);
        }

        clear();

        return ans;
    }
}
