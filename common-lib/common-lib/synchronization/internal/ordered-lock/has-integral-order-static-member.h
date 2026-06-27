#pragma once
#include <common-lib/mpl/type-transform.h>
#include <type_traits>

namespace vshalygin::cl::internal {
    template<typename T, typename Enable = void>
    struct has_integral_order_static_member
        : public std::false_type
    {};

    template<typename T>
    struct has_integral_order_static_member<T,
                   std::enable_if_t<std::is_integral_v<
                            decltype(std::declval<remove_type_qualifiers_t<T> &>().order)>>>
        : public std::true_type
    {};

    template<typename T>
    inline constexpr bool has_integral_order_static_member_t =
        has_integral_order_static_member<T>::value;
}
