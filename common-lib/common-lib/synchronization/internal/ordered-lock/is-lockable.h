#pragma once
#include <common-lib/mpl/type-transform.h>
#include <type_traits>

namespace vshalygin::cl::internal {
    namespace is_lockable_impl {
        template<typename T, typename Enable = void>
        struct is_lockable
            : std::false_type
        {
        };

        template<typename T>
        struct is_lockable<T, std::void_t<decltype(std::declval<T &>().lock()),
            decltype(std::declval<T &>().unlock())>>
            : std::true_type
        {};
    }

    template<typename T>
    inline constexpr bool is_lockable_v =
        is_lockable_impl::is_lockable<remove_type_qualifiers_t<T>>::value;
}
