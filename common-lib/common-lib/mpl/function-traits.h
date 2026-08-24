#pragma once
#include "internal/function-traits/function-traits.h"
#include "internal/function-traits/is-function-pointer.h"
#include "internal/function-traits/is-std-function.h"

namespace vshalygin::cl {
    template<size_t N, typename F>
    using function_arg_t =
        typename internal::function_traits<F>::template arg<N>;

    template<typename F>
    using function_ret_t =
        typename internal::function_traits<F>::ret;

    template<typename F>
    using function_class_t =
        typename internal::function_traits<F>::class_t;

    template<typename F>
    using function_signature_t =
        typename internal::function_traits<F>::signature;

    template<typename F>
    using function_args_as_tuple_t =
        typename internal::function_traits<F>::arg_as_tuple;

    template<typename F>
    inline constexpr size_t function_arg_count_v =
        internal::function_traits<F>::arg_count;

    template<typename T>
    inline constexpr bool is_function_pointer_v =
        internal::is_function_pointer_v<T>;

    template<typename T>
    inline constexpr bool is_std_function_v =
        internal::is_std_function_v<T>;
}
