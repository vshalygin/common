#pragma once
#include "remove-mem-func-qualifiers.h"

#include <tuple>
#include <type_traits>

namespace vshalygin::common {
    namespace internal {
        template<typename Ret, typename...Args>
        struct function_traits_base
        {
            using ret = Ret;
            using args = std::tuple<Args...>;

            static constexpr const unsigned args_num = sizeof...(Args);
        };

        template<typename Callable>
        struct function_traits_impl;

        template<typename Class, typename Ret, typename...Args>
        struct function_traits_impl<Ret(Class::*)(Args...)>
            : public function_traits_base<Ret, Args...>
        {};

        template<typename Class, typename Ret, typename...Args>
        struct function_traits_impl<Ret(Class::*)(Args......)>
            : public function_traits_base<Ret, Args...>
        {};

        template<typename Callable>
        struct decay_operator
        {
            using type = remove_mem_func_q_t<decltype(&std::remove_cvref_t<Callable>::operator())>;
        };
    }

    template<typename Callable>
    struct function_traits
        : public internal::function_traits_impl<typename internal::decay_operator<Callable>::type>
    {};
}
