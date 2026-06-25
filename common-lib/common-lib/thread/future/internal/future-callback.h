#pragma once
#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/function-traits.h>

#include <type_traits>

namespace vshalygin::cl::internal {
    template<typename T, typename Enable = void>
    class ifuture_callback;

    template<typename T>
    class ifuture_callback<T, std::enable_if_t<!std::is_same_v<T, void>>>
    {
    public:
        virtual ~ifuture_callback() = default;

        virtual void call(T &&val) = 0;
    };

    template<typename T, typename Func, typename Enable = void>
    class future_callback;

    template<typename T, typename Func>
    class future_callback<T, Func,
                std::enable_if_t<!std::is_same_v<T, void>>>
        : public ifuture_callback<T>
    {
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
