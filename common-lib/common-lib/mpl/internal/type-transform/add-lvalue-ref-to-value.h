#pragma once
#include <type_traits>

namespace vshalygin::cl::internal {
    template<typename T>
    struct add_lvalue_ref_to_value
    {
        using type = std::conditional_t<std::is_reference_v<T>, T, T &>;
    };

    template<>
    struct add_lvalue_ref_to_value<void>
    {
        using type = void;
    };

    template<typename T>
    using add_lvalue_ref_to_value_t =
        typename add_lvalue_ref_to_value<T>::type;
}
