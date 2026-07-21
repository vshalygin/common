#pragma once
#include "order-comparator.h"
#include "is-ordered-lockable-types-valid.h"
#include "ordered-lock-functions.h"

#include <common-lib/utils/tuple-utils.h>

namespace vshalygin::cl::internal {
    struct defer_lock_t
    {};

    struct adopt_lock_t
    {};

    template<typename...OrderedLockable>
    class ordered_lock
    {
        static_assert(is_ordered_lockable_types_valid_v<OrderedLockable...>,
                      "ordered lockable types are invalid");

        template<typename...OrderedLockable2>
        friend class ordered_lock;

        template<typename...Lockables, typename...AddLockables>
        friend ordered_lock<Lockables..., AddLockables...> do_push_back
                                     (ordered_lock<Lockables...> &&lock,
                                      AddLockables&...add_locables);

        using init_ptr_tuple = std::tuple<std::add_pointer_t<OrderedLockable>...>;
        using init_ptr_ref_tuple = std::tuple<std::add_pointer_t<OrderedLockable> &...>;
        using ordered_ptr_tuple = sort_tuple_t<init_ptr_tuple, order_comparator>;
        using ordered_ptr_ref_tuple = sort_tuple_t<init_ptr_ref_tuple, order_comparator>;

    public:
        ordered_lock() noexcept;

        explicit ordered_lock(OrderedLockable&...lockables);
        explicit ordered_lock(defer_lock_t, OrderedLockable&...lockables) noexcept;
        explicit ordered_lock(adopt_lock_t, OrderedLockable&...lockables) noexcept;

        ~ordered_lock();

        ordered_lock(const ordered_lock &) = delete;
        ordered_lock &operator=(const ordered_lock &) = delete;

        template<typename...OrderedLockable2,
                 std::enable_if_t<std::is_same_v<
                     sort_tuple_t<std::tuple<OrderedLockable...>, order_comparator>,
                     sort_tuple_t<std::tuple<OrderedLockable2...>, order_comparator>>, int> = 0>
        ordered_lock(ordered_lock<OrderedLockable2...> &&other)
            : ordered_lock()
        {
            m_is_locked = other.m_is_locked;
            m_ordered_ptr_ref_tuple = other.m_ordered_ptr_ref_tuple;

            other.clear();
        }

        template<typename...OrderedLockable2,
            std::enable_if_t<std::is_same_v<
                sort_tuple_t<std::tuple<OrderedLockable...>, order_comparator>,
                sort_tuple_t<std::tuple<OrderedLockable2...>, order_comparator>>, int> = 0>
        ordered_lock &operator=(ordered_lock<OrderedLockable2...> &&other)
        {
            if constexpr(std::is_same_v<std::tuple<OrderedLockable2...>,
                                        std::tuple<OrderedLockable...>>)
            {
                if(this == &other) {
                    return *this;
                }
            }

            if(is_locked()) {
                unlock();
            }

            m_is_locked = other.m_is_locked;
            m_ordered_ptr_ref_tuple = other.m_ordered_ptr_ref_tuple;

            other.clear();

            return *this;
        }

        void lock();
        void unlock() noexcept;
        bool is_locked() const noexcept;

        explicit operator bool() const noexcept;

    private:
        void clear() noexcept;

        template<size_t...I>
        void do_safe_lock(std::index_sequence<I...>);

        template<typename...AddLockables>
        ordered_lock<OrderedLockable..., AddLockables...> push_back
                                                   (AddLockables&...add_locables);
        template<typename...AddLockables, size_t...I>
        ordered_lock<OrderedLockable..., AddLockables...> push_back_impl
                                                   (AddLockables&...add_locables,
                                                    std::index_sequence<I...>);

    private:
        init_ptr_tuple m_init_ptr_tuple; 
        ordered_ptr_ref_tuple m_ordered_ptr_ref_tuple;
        bool m_is_locked = false;
    };

    template<typename...OrderedLockable>
    ordered_lock(OrderedLockable&...) -> ordered_lock<OrderedLockable...>;

    template<typename...OrderedLockable>
    ordered_lock(defer_lock_t, OrderedLockable&...) -> ordered_lock<OrderedLockable...>;

    template<typename...OrderedLockable>
    ordered_lock(adopt_lock_t, OrderedLockable&...) -> ordered_lock<OrderedLockable...>;
}
