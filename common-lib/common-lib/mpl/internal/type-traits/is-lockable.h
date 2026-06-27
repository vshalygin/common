#pragma once
#include <type_traits>

namespace vshalygin::cl::internal {
    template<typename T, typename Enable = void>
    struct is_lockable
        : std::false_type
    {};

    template<typename T>
    struct is_lockable<T, std::void_t<decltype(std::declval<T &>().lock()),
                                      decltype(std::declval<T &>().unlock())>>
        : std::true_type
    {};

    template<typename T>
    inline constexpr bool is_lockable_v = is_lockable<T>::value;
}
