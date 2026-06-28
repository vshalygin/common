#pragma once
#include "../type-transform/remove-type-qualifiers.h"

namespace vshalygin::cl::internal {
    template<typename T>
    inline constexpr bool is_function_pointer_v =
        std::is_pointer_v<remove_type_qualifiers_t<T>> &&
        std::is_function_v<std::remove_pointer_t<remove_type_qualifiers_t<T>>>;
}
