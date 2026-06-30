#pragma once
#include "has-integral-order-static-member.h"
#include "is-lockable.h"
#include "is-there-repeating-order-number.h"
#include "order-comparator.h"

namespace vshalygin::cl::internal {
    template<typename...OrderedLockable>
    class ordered_lock
    {
        static_assert((std::is_same_v<OrderedLockable,
                      remove_type_qualifiers_t<OrderedLockable>> && ...),
                      "no type qualifiers allowed");
        static_assert((is_lockable_v<OrderedLockable> && ...),
                      "all types must have lock() and unlock()");
        static_assert((has_integral_order_static_member_v<OrderedLockable> && ...),
                      "all ordered types must have integral static member named 'order'");
        static_assert(sizeof...(OrderedLockable) > 0),
                      "must be at least one lockable object");
        static_assert(!is_there_repeating_order_number_v<OrderedLockable...>),
                      "repeating order number are not allowed");

        using init_ptr_tuple = std::tuple<std::add_pointer_t<OrderedLockable>...>;
        using ordered_ptr_tuple = sort_tuple_t<init_ptr_tuple, order_comparator>;

    public:
        ordered_lock() = default;
        explicit ordered_lock(OrderedLockable&...lockables);

        ~ordered_lock();

        ordered_lock(const ordered_lock &) = delete;
        ordered_lock &operator=(const ordered_lock &) = delete;

        ordered_lock(ordered_lock<OrderedLockable...> &&other);
        ordered_lock &operator=(ordered_lock<OrderedLockable...> &&other);

        void lock();
        void unlock() noexcept;
        bool is_locked() const noexcept;

        operator bool() const noexcept;

    private:
        void clear() noexcept;

        template<size_t...I>
        void do_safe_lock(std::index_sequence<I...>);

    private:
        bool m_is_locked = false;
        ordered_ptr_tuple m_ordered_ptr_tuple;
    };
}
