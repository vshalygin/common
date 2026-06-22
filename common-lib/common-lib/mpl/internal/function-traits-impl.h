#pragma once
#include "../type-traits.h"
#include "../type-transform.h"

#include <tuple>
#include <type_traits>

namespace vshalygin::cl::internal {
    class no_type
    {
    };

    template<typename F>
    struct function_traits_base;

    template<typename C, typename R, typename...Args>
    struct function_traits_base<R(C:: *)(Args...)>
    {
        using ret = R;
        using class_t = C;

        static constexpr const size_t arg_count = sizeof...(Args);

        using arg_as_tuple = std::tuple<Args...>;

        template<size_t N>
        using arg = std::tuple_element_t<N, arg_as_tuple>;
    };

    template<typename R, typename...Args>
    struct function_traits_base<R(Args...)>
    {
        using ret = R;
        using class_t = no_type;

        static constexpr const size_t arg_count = sizeof...(Args);

        using arg_as_tuple = std::tuple<Args...>;

        template<size_t N>
        using arg = std::tuple_element_t<N, arg_as_tuple>;
    };

    template<typename F, typename Enable = void>
    struct function_traits;

    template<typename F>
    struct function_traits<F, std::enable_if_t<std::is_member_function_pointer_v<F>>>
        : public function_traits_base<remove_member_function_qualifiers_t<F>>
    {};

    template<typename F>
    struct function_traits<F, std::void_t<decltype(&F::operator())>>
        : public function_traits<decltype(&F::operator())>
    {};

    template<typename F>
    struct function_traits<F, std::enable_if_t<std::is_function_v<F>>>
        : public function_traits_base<remove_function_qualifiers_t<F>>
    {};

    template<typename F>
    struct function_traits<F, std::enable_if_t<is_function_pointer_v<F>>>
        : public function_traits_base<
        remove_function_qualifiers_t<std::remove_pointer_t<F>>>
    {};
}
