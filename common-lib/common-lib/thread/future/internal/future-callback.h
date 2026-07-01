#pragma once
#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/function-traits.h>

#include <type_traits>

namespace vshalygin::cl::internal {
    template<typename T>
    class ifuture_callback
    {
    public:
        virtual ~ifuture_callback() = default;

        virtual void call(T &&val) = 0;
    };

    template<typename T, typename Func>
    class future_callback
        : public ifuture_callback<T>
    {
        static_assert(std::is_same_v<function_ret_t<Func>, void>,
                      "internal: future_callback must have void return type");
        static_assert(function_arg_count_v<Func> == 1,
                      "internal: future_callback must have one argument");

    public:
        template<typename F>
        explicit future_callback(F &&f)
            : m_func(std::forward<F>(f))
        {}

        void call(T &&val) override
        {
            m_func(static_cast<function_arg_t<0, Func>>(val));
        }

    private:
        remove_type_qualifiers_t<Func> m_func;
    };
}
