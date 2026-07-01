#pragma once
#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/function-traits.h>

namespace vshalygin::cl::internal {
    template<typename Ret, typename...Args>
    class ipromise_function
    {
    public:
        virtual ~ipromise_function() = default;

        virtual Ret call(Args...args) = 0;
    };

    template<typename Func, typename Ret, typename TupleArgs>
    class promise_function_base;

    template<typename Func, typename Ret, typename...Args>
    class promise_function_base<Func, Ret, std::tuple<Args...>>
        : public ipromise_function<Ret, Args...>
    {
    public:
        explicit promise_function_base(Func &&func)
            : m_func(std::forward<Func>(func))
        {}

        promise_function_base(const promise_function_base &) = delete;
        promise_function_base &operator=(const promise_function_base &) = delete;

        Ret call(Args...args) override
        {
            return m_func(std::forward<Args>(args)...);
        }

    private:
        remove_type_qualifiers_t<Func> m_func;
    };

    template<typename Func>
    class promise_function
        : public promise_function_base<Func, function_ret_t<Func>, function_args_as_tuple_t<Func>>
    {
        using base_type = promise_function_base<Func, function_ret_t<Func>, function_args_as_tuple_t<Func>>;

    public:
        promise_function(Func &&func)
            : base_type(std::forward<Func>(func))
        {}

        promise_function(const promise_function &) = delete;
        promise_function &operator=(const promise_function &) = delete;
    };
}
