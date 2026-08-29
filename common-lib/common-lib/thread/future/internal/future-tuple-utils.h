#pragma once
#include <common-lib/mpl/type-transform.h>

#include <type_traits>

namespace vshalygin::cl::internal {
    template<typename...Args>
    class ftuple;

    template<typename T>
    struct is_nested_future_tuple
        : std::false_type
    {};

    template<typename...Args>
    struct is_nested_future_tuple<ftuple<Args...>>
        : std::true_type
    {};

    template<typename T>
    struct is_future_tuple_impl
        : std::false_type
    {};

    template<typename...Args>
    struct is_future_tuple_impl<ftuple<Args...>>
        : std::true_type
    {};

    template<typename T>
    struct is_future_tuple
        : is_future_tuple_impl<remove_type_qualifiers_t<T>>
    {};

    template<typename T>
    constexpr bool is_future_tuple_v = is_future_tuple<T>::value;

    template<typename T>
    struct future_tuple_has_reference_impl
        : std::false_type
    {};

    template<typename...Args>
    struct future_tuple_has_reference_impl<ftuple<Args...>>
        : std::bool_constant<(std::is_reference_v<Args> || ...)>
    {};

    template<typename T>
    struct future_tuple_has_reference
        : future_tuple_has_reference_impl<remove_type_qualifiers_t<T>>
    {};

    template<typename T>
    constexpr bool future_tuple_has_reference_v =
        future_tuple_has_reference<T>::value;
}
