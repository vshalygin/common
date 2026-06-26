#pragma once
#include <type_traits>

namespace vshalygin::cl::internal {
    template<typename From, typename To, typename = void>
    struct is_lvalue_static_castable
        : std::false_type
    {};

    template<typename From, typename To>
    struct is_lvalue_static_castable<From, To,
                      std::void_t<decltype(static_cast<To>(std::declval<From &>()))>>
        : std::true_type
    {};

    template<typename From, typename To>
    inline constexpr bool is_lvalue_static_castable_v =
        is_lvalue_static_castable<From, To>::value;
}
