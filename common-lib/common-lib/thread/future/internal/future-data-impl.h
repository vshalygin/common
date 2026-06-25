#pragma once
#include "future-controller.h"
#include <common-lib/mpl/type-traits.h>

namespace vshalygin::cl::internal {
    //TODO check
    template<typename T, typename ThreadPool>
    template<typename Func>
    void future_data<T, ThreadPool>::apply(Func &&func) const
    {
        static_assert(std::is_same_v<function_ret_t<Func>, void>);
        static_assert(function_arg_count_v<Func> == 1);
        static_assert(is_static_castable<const T &, function_arg_t<0, Func>>,
                      "unable to convert stored type to function parameter");

        std::lock_guard guard(m_controller->m_mtx);
        if constexpr(std::is_reference_v<arg>) {
            func(std::forward<arg>(m_controller->get_val()));
        } else {
            //copy value for handler non-reference parameter
            func(m_controller->get_val());
        }
    }

    //TODO check
    template<typename T, typename ThreadPool>
    template<typename Func>
    void future_data<T, ThreadPool>::apply(Func &&func)
    {
        static_assert(std::is_same_v<function_ret_t<Func>, void>);
        static_assert(function_arg_count_v<Func> == 1);

        using arg = function_arg_t<0, Func>;

        static_assert(!(!std::is_reference_v<arg> &&
                      std::is_volatile_v<std::remove_reference_t<arg>>),
                      "volatile non-reference value parameter is not allowed");
        static_assert(std::is_same_v<remove_c_ref_t<arg>,
                      remove_c_ref_t<T>>);


        std::lock_guard guard(m_controller->m_mtx);
        if constexpr(std::is_reference_v<arg>) {
            func(std::forward<arg>(m_controller->get_val()));
        } else {
            //copy value for handler parameter
            func(m_controller->get_val());
        }
    }
}
