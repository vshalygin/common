#pragma once
#include <tuple>

namespace vshalygin::common {
    namespace internal {
        template<typename Ret, typename...Args>
        struct function_traits_base
        {
            using ret = Ret;
            using args = std::tuple<Args...>;
        };

        template<typename Callable>
        struct function_traits_impl;

        template<typename Class, typename Ret, typename...Args>
        struct function_traits_impl<Ret(Class::*)(Args...) const>
            : public function_traits_base<Ret, Args...>
        {};
    }

    template<typename Callable>
    struct function_traits
        : public internal::function_traits_impl<decltype(&Callable::operator())>
    {};
}
