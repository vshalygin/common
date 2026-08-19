#pragma once
#include "../type-transform/remove-type-qualifiers.h"

namespace vshalygin::cl::internal {
    template<typename T>
    inline constexpr bool is_const_v =
        std::is_const_v<std::remove_reference_t<T>>;

    template<typename T>
    inline constexpr bool is_value_v =
        std::is_same_v<T, remove_type_qualifiers_t<T>>;

    template<typename T>
    inline constexpr bool is_lvalue_ref_v =
        std::is_same_v<T, remove_type_qualifiers_t<T> &>;

    template<typename T>
    inline constexpr bool is_rvalue_ref_v =
        std::is_same_v<T, remove_type_qualifiers_t<T> &&>;

    template<typename T>
    inline constexpr bool is_const_value_v =
        std::is_same_v<T, const remove_type_qualifiers_t<T>>;

    template<typename T>
    inline constexpr bool is_const_lvalue_ref_v =
        std::is_same_v<T, const remove_type_qualifiers_t<T> &>;

    template<typename T>
    inline constexpr bool is_const_rvalue_ref_v =
        std::is_same_v<T, const remove_type_qualifiers_t<T> &&>;

    template<typename T>
    inline constexpr bool is_volatile_value_v =
        std::is_same_v<T, volatile remove_type_qualifiers_t<T>>;

    template<typename T>
    inline constexpr bool is_volatile_lvalue_ref_v =
        std::is_same_v<T, volatile remove_type_qualifiers_t<T> &>;

    template<typename T>
    inline constexpr bool is_volatile_rvalue_ref_v =
        std::is_same_v<T, volatile remove_type_qualifiers_t<T> &&>;

    template<typename T>
    inline constexpr bool is_const_volatile_value_v =
        std::is_same_v<T, const volatile remove_type_qualifiers_t<T>>;

    template<typename T>
    inline constexpr bool is_const_volatile_lvalue_ref_v =
        std::is_same_v<T, const volatile remove_type_qualifiers_t<T> &>;

    template<typename T>
    inline constexpr bool is_const_volatile_rvalue_ref_v =
        std::is_same_v<T, const volatile remove_type_qualifiers_t<T> &&>;
}
