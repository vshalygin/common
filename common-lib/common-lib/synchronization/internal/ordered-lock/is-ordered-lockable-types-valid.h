#pragma once
#include "has-integral-order-static-member.h"
#include "is-lockable.h"
#include "is-there-repeating-order-number.h"

#include <common-lib/mpl/type-transform.h>

namespace vshalygin::cl::internal {
    template<typename...T>
    constexpr bool is_ordered_lockable_types_valid_v =
        (sizeof...(T) > 0) &&
        (std::is_same_v<T, remove_type_qualifiers_t<T>> && ...) &&
        (is_lockable_v<T> && ...) &&
        (has_integral_order_static_member_v<T> && ...) &&
        !is_there_repeating_order_number_v<T...>;
}