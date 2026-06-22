#pragma once
#include "internal/future-impl.h"

#include <functional>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <memory>

namespace vshalygin::cl {
    class thread_pool;

    template<typename T, typename ThreadPool = thread_pool>
    using future = internal::future_impl<T, ThreadPool>;

    template<typename T, typename ThreadPool = thread_pool>
    class promise
        : private internal::promise_impl<T, ThreadPool>
    {
        using base_type = internal::promise_impl<T, ThreadPool>;

    public:
        promise() = default;

        template<typename Function>
        explicit promise(ThreadPool *thread_pool,
                         Function &&function)
            : base_type(thread_pool, std::forward<Function>(function))
        {}

        future<T, ThreadPool> resolve()
        {
            return base_type::resolve();
        }

        bool is_valid() const
        {
            return base_type::is_valid();
        }
    };

    template<typename F, typename ThreadPool = thread_pool>
    promise(ThreadPool *, F &&) -> promise<function_ret_t<F>, ThreadPool>;
}
