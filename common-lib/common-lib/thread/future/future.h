#pragma once
#include "internal/future-impl.h"
#include "internal/promise-impl.h"
#include "internal/future-data-impl.h"
#include "internal/make-promise.h"
#include "internal/future-tuple.h"

namespace vshalygin::cl {
    template<typename T, typename ThreadPool>
    using future_data = internal::future_data<T, ThreadPool>;

    template<typename ThreadPool, typename T>
    using future = internal::future<ThreadPool, T>;

    template<typename ThreadPool, typename T, typename...ResolveArgs>
    using promise = internal::promise<ThreadPool, T, ResolveArgs...>;

    template<typename Func, typename ThreadPool>
    auto make_promise(ThreadPool *thread_pool, Func &&func)
    {
        return internal::do_make_promise(thread_pool, std::forward<Func>(func));
    }

    template<typename...Args>
    using future_tuple = internal::future_tuple<Args...>;
}
