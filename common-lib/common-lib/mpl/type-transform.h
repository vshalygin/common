#pragma once
#include "internal/remove-member-function-qualifiers.h"
#include "internal/remove-function-qualifiers.h"
#include "internal/make-function-type.h"
#include "internal/remove-function-qualifiers.h"
#include "internal/remove-c-ref.h"

#include <type_traits>

namespace vshalygin::cl {
    template<typename T>
    using remove_member_function_qualifiers_t =
        internal::remove_member_function_qualifiers_t<T>;

    template<typename T>
    using remove_function_qualifiers_t =
        internal::remove_function_qualifiers_t<T>;

    template<typename T>
    using remove_type_qualifiers_t = internal::remove_type_qualifiers_t<T>;

    template<typename T>
    using make_function_type_t = internal::make_function_type_t<T>;

    template<typename T>
    using remove_c_ref_t = internal::remove_c_ref_t<T>;
}
