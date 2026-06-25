#pragma once
#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/function-traits.h>

namespace vshalygin::cl::internal {
    template<typename Ret>
    class ipromise_function
    {
    public:
        virtual ~ipromise_function() = default;

        virtual Ret call() = 0;
    };

    template<typename Func>
    class promise_function
        : public ipromise_function<function_ret_t<Func>>
    {
    public:
        template<typename F>
        explicit promise_function(F &&func)
            : m_func(std::forward<F>(func))
        {
            static_assert(std::is_constructible_v<Func, F>);
        }

        promise_function(const promise_function &) = delete;
        promise_function &operator=(const promise_function &) = delete;

        function_ret_t<Func> call() override
        {
            return m_func();
        }

    private:
        remove_type_qualifiers_t<Func> m_func;
    };
}
