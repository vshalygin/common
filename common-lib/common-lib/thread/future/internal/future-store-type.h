#pragma once
#include "future.h"
#include <common-lib/mpl/type-transform.h>

namespace vshalygin::cl::internal {
    template<typename Future>
    struct future_store_type_base;

    template<typename ThreadPool, typename T>
    struct future_store_type_base<future<ThreadPool, T>>
    {
        using type = T;
    };

    template<typename Future>
    struct future_store_type
        : future_store_type_base<remove_type_qualifiers_t<Future>>
    {};

    template<typename Future>
    using future_store_type_t = typename future_store_type<Future>::type;
}
