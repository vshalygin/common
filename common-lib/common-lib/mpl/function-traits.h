#pragma once
#include "remove-member-function-qualifiers.h"

#include <tuple>

namespace vshalygin::cl {
    class no_type
    {};

    namespace internal {
        template<typename T>
        inline constexpr bool is_function_pointer_v =
            std::is_pointer_v<T> &&
            std::is_function_v<std::remove_pointer_t<T>>;

        template<typename T>
        struct remove_function_qualifiers;

        template<typename R, typename... Args>
        struct remove_function_qualifiers<R(Args...)>
        {
            using type = R(Args...);
        };

        template<typename R, typename... Args>
        struct remove_function_qualifiers<R(Args...) noexcept>
        {
            using type = R(Args...);
        };

        template<typename T>
        using remove_function_qualifiers_t =
            typename remove_function_qualifiers<T>::type;

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
    }

    template<typename F, typename Enable = void>
    struct function_traits;

    template<typename F>
    struct function_traits<F, std::enable_if_t<std::is_member_function_pointer_v<F>>>
        : public internal::function_traits_base<remove_member_function_qualifiers_t<F>>
    {};

    template<typename F>
    struct function_traits<F, std::void_t<decltype(&F::operator())>>
        : public function_traits<decltype(&F::operator())>
    {};

    template<typename F>
    struct function_traits<F, std::enable_if_t<std::is_function_v<F>>>
        : public internal::function_traits_base<internal::remove_function_qualifiers_t<F>>
    {};

    template<typename F>
    struct function_traits<F, std::enable_if_t<internal::is_function_pointer_v<F>>>
        : public internal::function_traits_base<
                     internal::remove_function_qualifiers_t<std::remove_pointer_t<F>>>
    {};

    template<size_t N, typename F>
    using function_arg_t = typename function_traits<F>::template arg<N>;

    template<typename F>
    using function_ret_t = typename function_traits<F>::ret;

    template<typename F>
    using function_class_t = typename function_traits<F>::class_t;

    template<typename F>
    using function_args_as_tuple_t = typename function_traits<F>::arg_as_tuple;

    template<typename F>
    inline constexpr size_t function_arg_count_v = function_traits<F>::arg_count;
}
