#pragma once
#include "future-controller.h"
#include "is-future-tuple.h"

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

        using val_t = std::tuple_element_t<1, decltype(m_controller->get_val())>;
        static_assert(std::is_reference_v<val_t>);

        if constexpr(is_future_tuple_v<std::remove_reference_t<val_t>>) {
            using future_tuple = std::remove_reference_t<val_t>;

            auto [guard, val] = m_controller->get_val();
            ::vshalygin::cl::internal::apply(func, std::forward<val_t>(val));
        } else {
            static_assert(function_arg_count_v<Func> == 1,
                          "function must have 1 parameter");

            using arg_t = function_arg_t<0, Func>;

            static_assert(is_lvalue_static_castable_v<val_t, arg_t>,
                          "unable to convert stored type to function parameter");

            auto [guard, val] = m_controller->get_val();
            func(static_cast<arg_t>(val));
        }
    }
}
