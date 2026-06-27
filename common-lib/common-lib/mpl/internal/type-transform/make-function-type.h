#pragma once
#include "../function-traits/function-traits.h"

namespace vshalygin::cl::internal {
    template<typename Ret, typename Tuple>
    struct make_function_type_base;

    template<typename Ret, typename... Args>
    struct make_function_type_base<Ret, std::tuple<Args...>>
    {
        using type = Ret(Args...);
    };

    template<typename Func>
    struct make_function_type
        : public make_function_type_base<typename function_traits<Func>::ret,
                                         typename function_traits<Func>::arg_as_tuple>
    {};

    template<typename T>
    using make_function_type_t =
        typename make_function_type<T>::type;
}
