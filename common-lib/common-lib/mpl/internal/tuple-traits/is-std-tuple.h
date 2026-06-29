#pragma once
#include "../type-transform/remove-type-qualifiers.h"

#include <tuple>
#include <type_traits>

namespace vshalygin::cl::internal {
    template<typename T>
    struct is_std_tuple_base
        : public std::false_type
    {};

    template<typename...Args>
    struct is_std_tuple_base<std::tuple<Args...>>
        : public std::true_type
    {};

    template<typename T>
    struct is_std_tuple
        : public is_std_tuple_base<remove_type_qualifiers_t<T>>
    {};

    template<typename T>
    inline constexpr bool is_std_tuple_v = is_std_tuple<T>::value;
}