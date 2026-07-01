#pragma once
#include "future-controller-impl.h"
#include "future-data-impl.h"

namespace vshalygin::cl::internal {
    template<typename T, typename ThreadPool>
    class promise;

    template<typename T, typename ThreadPool>
    class future
    {
        friend class promise<T, ThreadPool>;

        explicit future(
            ThreadPool *thread_pool,
            std::shared_ptr<future_controller<T, ThreadPool>> controller);

    public:
        future() = default;

        future(const future &) = delete;
        future &operator=(const future &) = delete;
        future(future &&) = default;
        future &operator=(future &&) = default;

        auto get() const;

        template<typename Func>
        future<function_ret_t<Func>, ThreadPool> then(Func &&task);

        void catched(std::function<void(std::exception_ptr)> &&task);
        future<T, ThreadPool> catch_and_release_itself(
                                   std::function<void(std::exception_ptr)> &&task);

        bool is_valid() const;

    private:
        ThreadPool *m_thread_pool = nullptr;
        std::shared_ptr<future_controller<T, ThreadPool>> m_controller;
    };
}
