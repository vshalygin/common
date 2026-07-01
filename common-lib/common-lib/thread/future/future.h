#pragma once
#include "internal/future-impl.h"
#include "internal/promise-impl.h"
#include "internal/future-data-impl.h"

#include <functional>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <memory>

namespace vshalygin::cl {
    class thread_pool;

    template<typename T, typename ThreadPool>
    class promise;

    template<typename T, typename ThreadPool = thread_pool>//TODO удалить дефолты
    using future_data = internal::future_data<T, ThreadPool>;

    template<typename T, typename ThreadPool = thread_pool>
    class future final
        : private internal::future<T, ThreadPool>
    {
        template<typename U, typename ThreadPool2>//TODO плохо!
        friend class promise;

        template<typename U, typename ThreadPool2>//TODO плохо!
        friend class future;

        using base_type = internal::future<T, ThreadPool>;

        future(base_type &&base)
            : base_type(std::move(base))
        {}

    public:
        future() = default;

        future(const future &) = delete;
        future &operator=(const future &) = delete;
        future(future &&) = default;
        future &operator=(future &&) = default;

        auto get() const
        {
            return base_type::get();
        }

        template<typename Func>
        future<function_ret_t<Func>, ThreadPool> then(Func &&task)
        {
            return base_type::template then<Func>(std::forward<Func>(task));
        }

        future<T, ThreadPool> &catched(
            std::function<void(std::exception_ptr)> &&task) &
        {
            base_type::catched(std::move(task));
            return *this;
        }

        future<T, ThreadPool> catched(
            std::function<void(std::exception_ptr)> &&task) &&
        {
            return base_type::catch_and_release_itself(std::move(task));
        }

        bool is_valid() const
        {
            return base_type::is_valid();
        }
    };

    template<typename T, typename ThreadPool = thread_pool>
    class promise final
        : private internal::promise<T, ThreadPool>
    {
        using base_type = internal::promise<T, ThreadPool>;

    public:
        promise() = default;

        template<typename Function>
        explicit promise(ThreadPool *thread_pool,
                         Function &&function)
            : base_type(thread_pool, std::forward<Function>(function))
        {}

        promise(const promise &) = delete;
        promise &operator=(const promise &) = delete;
        promise(promise &&) = default;
        promise &operator=(promise &&) = default;

        void resolve()
        {
            base_type::resolve();
        }

        future<T, ThreadPool> get_future()
        {
            return base_type::get_future();
        }

        bool is_valid() const
        {
            return base_type::is_valid();
        }
    };

    template<typename F, typename ThreadPool = thread_pool>
    promise(ThreadPool *, F &&) -> promise<function_ret_t<F>, ThreadPool>;
}
