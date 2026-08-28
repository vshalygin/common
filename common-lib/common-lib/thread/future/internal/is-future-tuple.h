#pragma once
#include "future-tuple.h"
#include <common-lib/mpl/type-transform.h>

namespace vshalygin::cl::internal {
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
}
