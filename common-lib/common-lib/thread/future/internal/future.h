#pragma once
#include "future-controller-impl.h"
#include "future-data-impl.h"

#include <common-lib/utils/function.h>

namespace vshalygin::cl::internal {
    template<typename ThreadPool, typename T, typename...ResolveArgs>
    class promise;

    template<typename ThreadPool, typename T>
    class future
    {
        template<typename, typename, typename...>
        friend class promise;

        template<typename, typename>
        friend class future;

        explicit future(
            ThreadPool *thread_pool,
            std::shared_ptr<future_controller<ThreadPool, T>> controller);

    public:
        future() = default;

        future(const future &) = delete;
        future &operator=(const future &) = delete;
        future(future &&) = default;
        future &operator=(future &&) = default;

        auto get() const;

        template<typename Func>
        auto then(Func &&task);

        template<typename Func>
        auto catched(Func &&task);

        bool is_valid() const;

    private:
        auto get_controller() const noexcept;

        template<typename Future>
        auto flatten_future(auto controller);

    private:
        ThreadPool *m_thread_pool = nullptr;
        std::shared_ptr<future_controller<ThreadPool, T>> m_controller;
    };
}
