#pragma once
#include <type_traits>

namespace vshalygin::cl::internal {
    template<typename T>
    struct remove_const
    {
        using type = std::remove_const_t<T>;
    };

    template<typename T>
    struct remove_const<T &>
    {
        using type = std::remove_const_t<T> &;
    };

    template<typename T>
    struct remove_const<T &&>
    {
        using type = std::remove_const_t<T> &&;
    };

    template<typename T>
    using remove_const_t = typename remove_const<T>::type;
}
