#pragma once
#include "internal/future-impl.h"
#include "internal/promise.h"
#include "internal/future-data.h"
#include "internal/make-promise.h"
#include "internal/make-future.h"
#include "internal/future-tuple.h"

namespace vshalygin::cl {
    using internal::future_data;

    using internal::future;
    using internal::promise;

    template<typename Func, typename ThreadPool>
    auto make_promise(ThreadPool *thread_pool, Func &&func)
    {
        return internal::do_make_promise(thread_pool, std::forward<Func>(func));
    }

    template<typename T, typename ThreadPool>
    auto make_ready_future(ThreadPool *thread_pool, T &&val)
    {
        return internal::do_make_ready_future(thread_pool, std::forward<T>(val));
    }

    template<typename ThreadPool>
    auto make_ready_future(ThreadPool *thread_pool)
    {
        return internal::do_make_ready_future(thread_pool);
    }


    using internal::ftuple;
}
