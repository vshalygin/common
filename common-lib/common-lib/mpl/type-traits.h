#pragma once
#include "internal/type-traits/check-type.h"
#include "internal/type-traits/is-lvalue-static-castable.h"
#include "internal/type-traits/is-lockable.h"

#include <type_traits>

namespace vshalygin::cl {
    template<typename T>
    inline constexpr bool is_lockable_v =
        internal::is_lockable_v<T>;

    template<typename From, typename To>
    inline constexpr bool is_lvalue_static_castable_v =
        internal::is_lvalue_static_castable_v<From, To>;

    template<typename T>
    inline constexpr bool is_value_v =
        internal::is_value_v<T>;

    template<typename T>
    inline constexpr bool is_lvalue_ref_v =
        internal::is_lvalue_ref_v<T>;

    template<typename T>
    inline constexpr bool is_rvalue_ref_v =
        internal::is_rvalue_ref_v<T>;

    template<typename T>
    inline constexpr bool is_const_v =
        internal::is_const_v<T>;

    template<typename T>
    inline constexpr bool is_const_value_v =
        internal::is_const_value_v<T>;

    template<typename T>
    inline constexpr bool is_const_lvalue_ref_v =
        internal::is_const_lvalue_ref_v<T>;

    template<typename T>
    inline constexpr bool is_const_rvalue_ref_v =
        internal::is_const_rvalue_ref_v<T>;

    template<typename T>
    inline constexpr bool is_volatile_value_v =
        internal::is_volatile_value_v<T>;

    template<typename T>
    inline constexpr bool is_volatile_lvalue_ref_v =
        internal::is_volatile_lvalue_ref_v<T>;

    template<typename T>
    inline constexpr bool is_volatile_rvalue_ref_v =
        internal::is_volatile_rvalue_ref_v<T>;

    template<typename T>
    inline constexpr bool is_const_volatile_value_v =
        internal::is_const_volatile_value_v<T>;

    template<typename T>
    inline constexpr bool is_const_volatile_lvalue_ref_v =
        internal::is_const_volatile_lvalue_ref_v<T>;

    template<typename T>
    inline constexpr bool is_const_volatile_rvalue_ref_v =
        internal::is_const_volatile_rvalue_ref_v<T>;
}
