#pragma once
#include "internal/tuple-traits/is-std-tuple.h"
#include "internal/tuple-traits/tuple-size.h"
#include "internal/tuple-traits/tuple-element.h"

namespace vshalygin::cl {
    template<typename T>
    inline constexpr bool is_std_tuple_v =
        internal::is_std_tuple_v<T>;

    template<typename Tuple>
    inline constexpr size_t tuple_size_v =
        internal::tuple_size_v<Tuple>;

    template<typename Tuple, std::size_t I>
    using tuple_element_t =
        internal::tuple_element_t<Tuple, I>;
}
