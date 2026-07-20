#pragma once
#include "internal/function-transform/remove-member-function-qualifiers.h"
#include "internal/function-transform/remove-function-qualifiers.h"
#include "internal/function-transform/make-function-type.h"

namespace vshalygin::cl {
    template<typename T>
    using remove_member_function_qualifiers_t =
        internal::remove_member_function_qualifiers_t<T>;

    template<typename T>
    using remove_function_qualifiers_t =
        internal::remove_function_qualifiers_t<T>;

    //any function-like to function type, skip any qualifiers
    //e.g. void(*&)->void(), void(C::*)() & noexcept -> void()
    template<typename T>
    using make_function_type_t =
        internal::make_function_type_t<T>;
}
