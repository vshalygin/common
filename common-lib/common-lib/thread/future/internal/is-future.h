#pragma once
#include "future.h"
#include <common-lib/mpl/type-transform.h>

namespace vshalygin::cl::internal {
    template<typename T>
    struct is_future_impl
        : std::false_type
    {};

    template<typename ThreadPool, typename T>
    struct is_future_impl<future<ThreadPool, T>>
        : std::true_type
    {};

    template<typename T>
    struct is_future
        : is_future_impl<remove_type_qualifiers_t<T>>
    {};

    template<typename T>
    constexpr bool is_future_v = is_future<T>::value;

    template<typename T>
    struct is_flattenable_future
        : is_future_impl<T>
    {};

    template<typename T>
    constexpr bool is_flattenable_future_v = is_flattenable_future<T>::value;
}
