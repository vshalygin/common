#pragma once
#include "../type-transform/remove-type-qualifiers.h"
#include "../type-transform/remove-function-qualifiers.h"
#include "../type-transform/remove-member-function-qualifiers.h"
#include "is-std-function.h"
#include "is-function-pointer.h"

#include <tuple>
#include <type_traits>

namespace vshalygin::cl::internal {
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

        static constexpr const size_t arg_count = sizeof...(Args);

        using arg_as_tuple = std::tuple<Args...>;

        template<size_t N>
        using arg = std::tuple_element_t<N, arg_as_tuple>;
    };

    template<typename R, typename...Args>
    struct function_traits_base<std::function<R(Args...)>>
    {
        using ret = R;
        using class_t = std::function<R(Args...)>;

        static constexpr const size_t arg_count = sizeof...(Args);

        using arg_as_tuple = std::tuple<Args...>;

        template<size_t N>
        using arg = std::tuple_element_t<N, arg_as_tuple>;
    };

    template<typename F, typename Enable = void>
    struct function_traits;

    template<typename F>
    struct function_traits<
      F, std::enable_if_t<std::is_member_function_pointer_v<remove_type_qualifiers_t<F>>>>
        : public function_traits_base<remove_member_function_qualifiers_t<remove_type_qualifiers_t<F>>>
    {};

    template<typename F>
    struct function_traits<F, std::void_t<
                                   decltype(&remove_type_qualifiers_t<F>::operator()),
                                   std::enable_if_t<!is_std_function<F>::value>>>
        : public function_traits<decltype(&remove_type_qualifiers_t<F>::operator())>
    {};

    template<typename F>
    struct function_traits<F, std::enable_if_t<is_std_function_v<F>>>
        : public function_traits_base<remove_type_qualifiers_t<F>>
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
