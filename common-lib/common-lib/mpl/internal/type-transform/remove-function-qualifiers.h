#pragma once

namespace vshalygin::cl::internal {
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
}
