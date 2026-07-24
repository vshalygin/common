#pragma once
#include "future-impl.h"
#include <common-lib/mpl/type-transform.h>

namespace vshalygin::cl::internal {
    template<typename T, typename ThreadPool>
    auto do_make_ready_future(ThreadPool *thread_pool, T &&val)
    {
        return future<ThreadPool, remove_type_qualifiers_t<T>>(thread_pool, std::forward<T>(val));
    }

    template<typename ThreadPool>
    auto do_make_ready_future(ThreadPool *thread_pool)
    {
        return future<ThreadPool, void>(thread_pool);
    }
}
