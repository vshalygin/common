#pragma once
#include "../type-transform/remove-type-qualifiers.h"
#include <tuple>

namespace vshalygin::cl::internal {
    template<typename Tuple1, typename Tuple2>
    struct merge_tuples_base
    {
        static_assert(sizeof(Tuple1) == 0,
                      "bad tuple types");
    };

    template<typename...Args1, typename...Args2>
    struct merge_tuples_base<std::tuple<Args1...>, std::tuple<Args2...>>
    {
        using type = std::tuple<Args1..., Args2...>;
    };

    template<typename Tuple1, typename Tuple2>
    struct merge_tuples
        : merge_tuples_base<remove_type_qualifiers_t<Tuple1>, remove_type_qualifiers_t<Tuple2>>
    {};

    template<typename Tuple1, typename Tuple2>
    using merge_tuples_t =
        typename merge_tuples<Tuple1, Tuple2>::type;
}
