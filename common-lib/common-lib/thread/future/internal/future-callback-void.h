#pragma once
#include "future-callback.h"

namespace vshalygin::cl::internal {
    template<>
    class ifuture_callback<void>
    {
    public:
        virtual ~ifuture_callback() = default;

        virtual void call() = 0;
    };

    template<typename Func>
    class future_callback<void, Func>
        : public ifuture_callback<void>
    {
        static_assert(std::is_same_v<function_ret_t<Func>, void>,
                      "internal: future_callback must have void return type");
        static_assert(function_arg_count_v<Func> == 0,
                      "internal: future_callback must have no argument");

    public:
        template<typename F>
        explicit future_callback(F &&f)
            : m_func(std::forward<F>(f))
        {}

        void call() override
        {
            m_func();
        }

    private:
        remove_type_qualifiers_t<Func> m_func;
    };
}
