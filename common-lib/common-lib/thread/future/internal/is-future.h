#pragma once
#include "future.h"
#include <common-lib/mpl/type-transform.h>

namespace vshalygin::cl::internal {
    template<typename T>
    struct is_future_base
        : std::false_type
    {};

    template<typename ThreadPool, typename T>
    struct is_future_base<future<ThreadPool, T>>
        : std::true_type
    {};
    
    template<typename T>
    struct is_future
        : is_future_base<remove_type_qualifiers_t<T>>
    {};

    template<typename T>
    constexpr bool is_future_v = is_future<T>::value;
}
