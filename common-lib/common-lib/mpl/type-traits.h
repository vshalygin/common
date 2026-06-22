#pragma once
#include <type_traits>

namespace vshalygin::cl {
    template<typename T>
    inline constexpr bool is_function_pointer_v =
        std::is_pointer_v<T> &&
        std::is_function_v<std::remove_pointer_t<T>>;
}
