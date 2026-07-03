#pragma once
#include "future.h"
#include <common-lib/mpl/type-transform.h>

namespace vshalygin::cl::internal {
    template<typename T>
    struct is_future
        : std::false_type
    {};

    template<typename ThreadPool, typename T>
    struct is_future<future<ThreadPool, T>>
        : std::true_type
    {};
   

    template<typename T>
    constexpr bool is_future_v = is_future<T>::value;
}
