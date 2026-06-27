#pragma once
#include "has-integral-order-static-member.h"

#include <common-lib/mpl/type-traits.h>
#include <common-lib/mpl/type-transform.h>
#include <common-lib/utils/tuple-utils.h>

namespace vshalygin::cl {

    struct order_comparator
    {
        template<typename T, typename U>
        static constexpr bool compare()
        {
            return T::order < U::order;
        }
    };

    template<typename...OrderedLockable>
    class ordered_lock final
    {
        static_assert((is_lockable<OrderedLockable>::value && ...),
                      "all types must have lock() and unlock()");
        static_assert((!std::is_reference_v<OrderedLockable> && ...),
                      "non-reference types are not allowed");
        static_assert((!std::is_volatile_v<std::remove_reference_t<OrderedLockable>> && ...),
                      "volatile types are not allowed");
        static_assert((!std::is_const_v<std::remove_reference_t<OrderedLockable>> && ...),
                      "const types are not allowed");
        static_assert((has_integral_order_static_member_t<OrderedLockable> && ...),
                      "all ordered types must have integral static member named 'order'");

        using init_tuple = std::tuple<OrderedLockable &...>;
        using ordered_tuple = sorted_tuple_t<order_comparator, init_tuple>;

    public:
        explicit ordered_lock(OrderedLockable&...lockables)
            : m_ordered_tuple(sort_tuple<order_comparator>(init_tuple{lockables...}))
        {}

        ordered_lock(const ordered_lock &) = delete;
        ordered_lock &operator(const ordered_lock &) = delete;



    private:
        ordered_tuple m_ordered_tuple;
    };
}
