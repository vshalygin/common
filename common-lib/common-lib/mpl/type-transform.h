#pragma once
#include "internal/type-transform/remove-member-function-qualifiers.h"
#include "internal/type-transform/remove-function-qualifiers.h"
#include "internal/type-transform/make-function-type.h"
#include "internal/type-transform/remove-function-qualifiers.h"
#include "internal/type-transform/remove-c-ref.h"
#include "internal/type-transform/remove-const.h"
#include "internal/type-transform/add-const.h"

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

    template<typename T>
    using add_const_t = internal::add_const_t<T>;

    template<typename T>
    using remove_const_t = internal::remove_const_t<T>;
}
