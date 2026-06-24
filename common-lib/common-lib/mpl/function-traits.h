#pragma once
#include "type-transform.h"
#include "internal/function-traits-impl.h"

namespace vshalygin::cl {
    //TODO рассмотреть случаи ссылки на функции

    template<size_t N, typename F>
    using function_arg_t = typename internal::function_traits<F>::template arg<N>;

    template<typename F>
    using function_ret_t = typename internal::function_traits<F>::ret;

    template<typename F>
    using function_class_t = typename internal::function_traits<F>::class_t;

    template<typename F>
    using function_args_as_tuple_t = typename internal::function_traits<F>::arg_as_tuple;

    template<typename F>
    inline constexpr size_t function_arg_count_v = internal::function_traits<F>::arg_count;
}
