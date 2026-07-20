#pragma once
#include "internal/type-transform/remove-c-ref.h"
#include "internal/type-transform/remove-const.h"
#include "internal/type-transform/add-const.h"
#include "internal/type-transform/add-lvalue-ref-to-value.h"
#include "internal/type-transform/remove-type-qualifiers.h"

namespace vshalygin::cl {
    template<typename T>
    using remove_type_qualifiers_t =
        internal::remove_type_qualifiers_t<T>;

    template<typename T>
    using remove_c_ref_t =
        internal::remove_c_ref_t<T>;

    template<typename T>
    using add_const_t =
        internal::add_const_t<T>;

    template<typename T>
    using remove_const_t =
        internal::remove_const_t<T>;

    template<typename T>
    using add_lvalue_ref_to_value_t =
        internal::add_lvalue_ref_to_value_t<T>;
}
