#pragma once
#include "internal/future-impl.h"
#include "internal/promise.h"
#include "internal/future-data.h"
#include "internal/make-promise.h"
#include "internal/future-tuple.h"

namespace vshalygin::cl {
    using internal::ftuple;

    using internal::future_data;

    using internal::future;
    using internal::promise;

    template<typename Func, typename ThreadPool>
    auto make_promise(ThreadPool *thread_pool, Func &&func)
    {
        return internal::do_make_promise(thread_pool, std::forward<Func>(func));
    }
}
