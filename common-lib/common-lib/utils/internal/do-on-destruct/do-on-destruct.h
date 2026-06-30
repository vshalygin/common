#pragma once
#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/function-traits.h>

namespace vshalygin::cl::internal {
    template<typename Func>
    class do_on_destruct
    {
        static_assert(std::is_same_v<function_ret_t<Func>, void>,
                      "function must return void");
        static_assert(function_arg_count_v<Func> == 0,
                      "function must have no arguments");

    public:
        do_on_destruct(Func &&func)
            : m_func(std::forward<Func>(func))
        {}

        ~do_on_destruct()
        {
            if(!m_is_released) try {
                m_func();
            } catch(...) {
            }
        }

        do_on_destruct(const do_on_destruct &) = delete;
        do_on_destruct &operator=(const do_on_destruct &) = delete;

        void release() noexcept
        {
            m_is_released = true;
        }

    private:
        bool m_is_released = false;
        remove_type_qualifiers_t<Func> m_func;
    };
}
