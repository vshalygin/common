#pragma once
#include "future-controller.h"
#include "future-data.h"

#include <common-lib/utils/function.h>

namespace vshalygin::cl::internal {
    template<typename ThreadPool, typename Signature>
    class promise;

    template<typename ThreadPool, typename T>
    class future
    {
        template<typename, typename>
        friend class promise;

        template<typename, typename>
        friend class future;

        explicit future(
            ThreadPool *thread_pool,
            std::shared_ptr<future_controller<ThreadPool, T>> controller);

    public:
        future() = default;

        template<typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
        explicit future(ThreadPool *thread_pool, U &&val);

        template<typename U = T, std::enable_if_t<std::is_void_v<U>, int> = 0>
        explicit future(ThreadPool *thread_pool);

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

        bool has_value() const;
        bool has_exception() const;

        bool is_valid() const noexcept;

    private:
        template<typename Func, typename ControllerSp,
                 typename U = T, std::enable_if_t<!std::is_void_v<U>, int> = 0>
        static void exec_then_on_success(ControllerSp controller,
                                         Func &&task,
                                         add_lvalue_ref_to_value_t<U> param);

        template<typename Func, typename ControllerSp,
                 typename U = T, std::enable_if_t<std::is_void_v<U>, int> = 0>
        static void exec_then_on_success(ControllerSp controller,
                                         Func &&task);

        template<typename Func, typename NewControllerWp>
        auto get_on_success_for_catched_method(NewControllerWp new_controller_wp) const;

        template<typename Func, typename NewControllerWp>
        auto get_on_fail_for_catched_method(Func &&func, NewControllerWp new_controller_wp) const;

        template<typename FuncSp, typename NewControllerWp>
        auto get_on_success_for_finally_method(FuncSp func_sp, NewControllerWp new_controller_wp) const;

        template<typename FuncSp, typename NewControllerWp>
        auto get_on_fail_for_finally_method(FuncSp func_sp, NewControllerWp new_controller_wp) const;

        auto get_controller() const noexcept;

        template<typename Future, typename Controller>
        auto flatten_future(std::shared_ptr<Controller> controller);

        template<typename U, typename ControllerSp>
        std::shared_ptr<future_controller<ThreadPool, U>> create_child_controller(ControllerSp controller);

    private:
        ThreadPool *m_thread_pool = nullptr;
        std::shared_ptr<future_controller<ThreadPool, T>> m_controller;
    };

    template<typename ThreadPool, typename T>
    future(ThreadPool *thread_pool, T &&val) -> future<ThreadPool, remove_type_qualifiers_t<T>>;

    template<typename ThreadPool>
    future(ThreadPool *thread_pool) -> future<ThreadPool, void>;
}
