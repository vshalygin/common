#pragma once
#include "future-tuple.h"

namespace vshalygin::cl::internal {
    template<typename T>
    struct is_future_tuple
        : std::false_type
    {};

    template<typename...Args>
    struct is_future_tuple<ftuple<Args...>>
        : std::true_type
    {};

    template<typename T>
    constexpr bool is_future_tuple_v = is_future_tuple<T>::value;
}
