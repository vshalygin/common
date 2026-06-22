#pragma once
#include "remove-type-qualifiers.h"
#include <type_traits>
#include <functional>

namespace vshalygin::cl::internal {
    template<typename T>
    struct is_std_function_base
        : public std::false_type
    {};

    template<typename R, typename...Args>
    struct is_std_function_base<std::function<R(Args...)>>
        : public std::true_type
    {};

    template<typename T>
    struct is_std_function
        : public is_std_function_base<remove_type_qualifiers_t<T>>
    {};

    template<typename T>
    inline constexpr bool is_std_function_v = is_std_function<T>::value;
}
