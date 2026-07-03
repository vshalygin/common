#pragma once
#include "promise-function.h"
#include "future-controller.h"
#include "future-store-type-or-self.h"

#include <memory>

namespace vshalygin::cl::internal {
    template<typename ThreadPool, typename T, typename...ResolveArgs>
    class promise
    {
        template<typename, typename, typename...>
        friend class promise;

    public:
        promise() = default;

        template<typename Function>
        explicit promise(ThreadPool *thread_pool,
                         Function &&function);

        promise(const promise &) = delete;
        promise &operator=(const promise &) = delete;

        promise(promise &&) = default;
        promise &operator=(promise &&) = default;

        void resolve(ResolveArgs...args);

        auto get_future();

        bool is_valid() const;

    private:
        ThreadPool *m_thread_pool = nullptr;

        //shared_ptr for thread pools, which don't accept move-only functors
        std::shared_ptr<ipromise_function<T, ResolveArgs...>> m_function;

        std::shared_ptr<future_controller<future_store_type_or_self_t<T>, ThreadPool>> m_controller;
        future<ThreadPool, future_store_type_or_self_t<T>> m_future;
    };
}
