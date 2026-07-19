#pragma once
#include "future-controller.h"
#include "future-data.h"

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

        void wait() const;
        bool wait_for(std::chrono::milliseconds timeout) const;

        template<typename Func>
        auto then(Func &&task);

        template<typename Func>
        auto catched(Func &&task);

        template<typename Func>
        auto finally(Func &&task);

        bool is_valid() const noexcept;

    private:
        template<typename Func,
                 typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
        static void exec_then_on_success(auto controller,
                                         Func &&task,
                                         add_lvalue_ref_to_value_t<U> param);

        template<typename Func,
                 typename U = T, std::enable_if_t<std::is_void_v<U>, int> = 0>
        static void exec_then_on_success(auto controller,
                                         Func &&task);

        auto get_controller() const noexcept;

        template<typename Future, typename Controller>
        auto flatten_future(std::shared_ptr<Controller> controller);

        template<typename U>
        std::shared_ptr<future_controller<ThreadPool, U>> create_child_controller(auto controller);

    private:
        ThreadPool *m_thread_pool = nullptr;
        std::shared_ptr<future_controller<ThreadPool, T>> m_controller;
    };
}
