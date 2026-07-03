#pragma once
#include "future.h"
#include <common-lib/mpl/type-transform.h>

namespace vshalygin::cl::internal {
    template<typename T>
    struct future_store_type_or_self
    {
        using type = T;
    };

    template<typename ThreadPool, typename T>
    struct future_store_type_or_self<future<ThreadPool, T>>
    {
        using type = T;
    };

    template<typename Future>
    using future_store_type_or_self_t =
        typename future_store_type_or_self<Future>::type;
}
