#pragma once
#include "internal/is-std-function.h"
#include "internal/is-function-pointer.h"
#include <type_traits>

namespace vshalygin::cl {
    template<typename T>
    inline constexpr bool is_function_pointer_v = internal::is_function_pointer_v;

    template<typename T>
    inline constexpr bool is_std_function_v = internal::is_std_function_v<T>;
}
