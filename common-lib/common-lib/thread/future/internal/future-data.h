#pragma once
#include "future-tuple.h"
#include "is-future-tuple.h"

#include <common-lib/mpl/function-traits.h>

#include <memory>

namespace vshalygin::cl::internal {
    template<typename ThreadPool, typename T>
    class future_controller;

    template<typename T, typename ThreadPool>
    class future_data final
    {
        friend class future_controller<ThreadPool, T>;

        using controller_t = future_controller<ThreadPool, T>;

        explicit future_data(std::shared_ptr<controller_t> controller);

    public:
        future_data(const future_data &) = delete;
        future_data &operator=(const future_data &) = delete;

        future_data(future_data &&) = default;
        future_data &operator=(future_data &&) = default;

        template<typename Func>
        void apply(Func &&func) const;

    private:
        std::shared_ptr<controller_t> m_controller;
    };

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
            auto [guard, val] = m_controller->get_val();
            ::vshalygin::cl::internal::apply(func, std::forward<val_t>(val));
        } else {
            static_assert(function_arg_count_v<Func> == 1,
                          "function must have 1 parameter");

            using arg_t = function_arg_t<0, Func>;

            auto [guard, val] = m_controller->get_val();
            func(type_qualifiers_cast<arg_t>(val));
        }
    }
}
