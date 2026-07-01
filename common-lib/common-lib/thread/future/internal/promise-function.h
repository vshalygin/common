#pragma once
#include <common-lib/mpl/type-transform.h>

namespace vshalygin::cl::internal {
    template<typename Ret, typename...Args>
    class ipromise_function
    {
    public:
        virtual ~ipromise_function() = default;

        virtual Ret call(Args...args) = 0;
    };

    template<typename Func, typename Ret, typename...Args>
    class promise_function
        : public ipromise_function<Ret, Args...>
    {
    public:
        explicit promise_function(Func &&func)
            : m_func(std::forward<Func>(func))
        {}

        promise_function(const promise_function &) = delete;
        promise_function &operator=(const promise_function &) = delete;

        Ret call(Args...args) override
        {
            return m_func(std::forward<Args>(args)...);
        }

    private:
        remove_type_qualifiers_t<Func> m_func;
    };
}
