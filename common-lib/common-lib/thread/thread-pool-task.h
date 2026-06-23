#pragma once
#include "internal/thread-pool-task-impl.h"

namespace vshalygin::cl {
    class thread_pool;

    template<typename Signature>
    class thread_pool_task final
        : private internal::thread_pool_task<Signature>
    {
        friend class thread_pool;

        static_assert(std::is_same_v<function_ret_t<Signature>, void>);

        using base_type = internal::thread_pool_task<Signature>;

    public:
        thread_pool_task() = default;

        template<typename Func,
                 std::enable_if_t<!std::is_same_v<
                               std::remove_cv_t<std::remove_reference_t<Func>>,
                               thread_pool_task<Signature>>, int> = 0>
        explicit thread_pool_task(Func &&func)
            : base_type(std::forward<Func>(func))
        {}

        thread_pool_task(const thread_pool_task &other) = default;
        thread_pool_task &operator=(const thread_pool_task &other) = default;
        thread_pool_task(thread_pool_task &&other) = default;
        thread_pool_task &operator=(thread_pool_task &&) = default;

        operator bool() const
        {
            return base_type::is_valid();
        }
    };

    template<typename T>
    inline constexpr bool is_thread_pool_task_v = internal::is_thread_pool_task_v<T>;


    template<typename Func,
             typename = std::enable_if_t<!is_thread_pool_task_v<Func>>>
    thread_pool_task(Func &&) -> thread_pool_task<make_function_type_t<Func>>;
}
