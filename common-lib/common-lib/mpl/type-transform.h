#pragma once
#include "internal/remove-member-function-qualifiers.h"
#include "internal/remove-function-qualifiers.h"

namespace vshalygin::cl {
    template<typename T>
    using remove_member_function_qualifiers_t =
        typename  internal::remove_member_function_qualifiers<T>::type;

    template<typename T>
    using remove_function_qualifiers_t =
        typename internal::remove_function_qualifiers<T>::type;
}
