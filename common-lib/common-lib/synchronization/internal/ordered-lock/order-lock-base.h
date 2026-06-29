#pragma once
#include "has-integral-order-static-member.h"
#include "order-comparator.h"
#include "is-lockable.h"

#include <common-lib/mpl/type-traits.h>
#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/tuple-transform.h>
#include <common-lib/mpl/tuple-traits.h>
#include <common-lib/utils/tuple-utils.h>

namespace vshalygin::cl::internal {
    template<typename...OrderedLockable>
    class ordered_lock_base
    {
        static_assert((std::is_same_v<OrderedLockable,
                      remove_type_qualifiers_t<OrderedLockable>> && ...),
                      "no type qualifiers allowed");
        static_assert((is_lockable_v<OrderedLockable> && ...),
                      "all types must have lock() and unlock()");
        static_assert((has_integral_order_static_member_v<OrderedLockable> && ...),
                      "all ordered types must have integral static member named 'order'");

        using init_ptr_tuple = std::tuple<std::add_pointer_t<OrderedLockable>...>;
        using ordered_ptr_tuple = sort_tuple_t<init_ptr_tuple, order_comparator>;

    protected:
        ordered_lock_base() = default;
        explicit ordered_lock_base(OrderedLockable&...lockables);
        explicit ordered_lock_base(const ordered_ptr_tuple &tuple);

        ordered_lock_base(const ordered_lock_base &) = default;
        ordered_lock_base &operator=(const ordered_lock_base &) = default;

        bool is_valid() const noexcept;

        void lock();
        void unlock();

        void clear() noexcept;

        void set_ordered_ptr_tuple(const ordered_ptr_tuple &ordered_ptr_tuple) noexcept
        {
            m_ordered_ptr_tuple = ordered_ptr_tuple;
        }

        const ordered_ptr_tuple &get_ordered_ptr_tuple() const noexcept
        {
            return m_ordered_ptr_tuple;
        }

    private:
        ordered_ptr_tuple m_ordered_ptr_tuple;
    };
}
