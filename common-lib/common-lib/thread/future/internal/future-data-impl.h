#pragma once
#include "future-controller.h"
#include <common-lib/mpl/type-traits.h>

namespace vshalygin::cl::internal {
    template<typename T, typename ThreadPool>
    future_data<T, ThreadPool>::future_data(std::shared_ptr<controller_t> controller)
        : m_controller(std::move(controller))
    {}

    template<typename T, typename ThreadPool>
    template<typename Func>
    void future_data<T, ThreadPool>::apply(Func &&func) const
    {
        static_assert(std::is_same_v<function_ret_t<Func>, void>,
                      "function return type is not void");
        static_assert(function_arg_count_v<Func> == 1,
                      "function must have 1 parameter");

        using arg_t = function_arg_t<0, Func>;

        static_assert(std::is_reference_v<decltype(m_controller->get_val())>);
        static_assert(is_static_castable_v<decltype(m_controller->get_val()), arg_t>,
                      "unable to convert stored type to function parameter");

        std::lock_guard guard(m_controller->m_mtx);
        func(static_cast<arg_t>(m_controller->get_val()));
    }

    template<typename T, typename ThreadPool>
    template<typename Func>
    void future_data<T, ThreadPool>::apply(Func &&func)
    {
        static_assert(std::is_same_v<function_ret_t<Func>, void>,
                      "function return type is not void");
        static_assert(function_arg_count_v<Func> == 1,
                      "function must have 1 parameter");

        using arg_t = function_arg_t<0, Func>;

        static_assert(std::is_reference_v<decltype(m_controller->get_val())>);
        static_assert(is_static_castable_v<decltype(m_controller->get_val()), arg_t>,
                      "unable to convert stored type to function parameter");


        std::lock_guard guard(m_controller->m_mtx);
        func(static_cast<arg_t>(m_controller->get_val()));
    }
}
