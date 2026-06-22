#pragma once
#include "internal/is-std-function.h"
#include <type_traits>

namespace vshalygin::cl {
    template<typename T>
    inline constexpr bool is_function_pointer_v =
        std::is_pointer_v<T> &&
        std::is_function_v<std::remove_pointer_t<T>>;

    template<typename T>
    inline constexpr bool is_std_function_v = internal::is_std_function<T>::value;
}
