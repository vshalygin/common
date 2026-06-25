#pragma once
#include "promise-function.h"
#include "future-controller.h"

#include <memory>

namespace vshalygin::cl::internal {
    template<typename T, typename ThreadPool>
    class future;

    template<typename T, typename ThreadPool>
    class promise
    {
        template<typename U, typename TP>
        friend class promise;

        template<typename U, typename TP>
        friend class future;

        explicit promise(ThreadPool *thread_pool);

    public:
        promise() = default;

        template<typename Function>
        explicit promise(ThreadPool *thread_pool,
                         Function &&function);

        promise(const promise &) = delete;
        promise &operator=(const promise &) = delete;

        promise(promise &&) = default;
        promise &operator=(promise &&) = default;

        void resolve();
        future<T, ThreadPool> get_future();

        bool is_valid() const;

    private:
        std::shared_ptr<future_controller<T, ThreadPool>> get_controller() const;

    private:
        ThreadPool *m_thread_pool = nullptr;

        //shared_ptr for thread pools, which don't accept move-only functors
        std::shared_ptr<ipromise_function<T>> m_function;

        std::shared_ptr<future_controller<T, ThreadPool>> m_controller;
        future<T, ThreadPool> m_future;
    };
}
