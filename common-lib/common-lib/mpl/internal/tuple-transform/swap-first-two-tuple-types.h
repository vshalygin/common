#pragma once
#include "../type-transform/remove-type-qualifiers.h"
#include <tuple>

namespace vshalygin::cl::internal {
    template<typename Tuple>
    struct swap_first_two_tuple_types_base
    {
        static_assert(sizeof(Tuple) == 0,
                      "invalid tuple type");
    };

    template<typename First, typename Second, typename...Args>
    struct swap_first_two_tuple_types_base<std::tuple<First, Second, Args...>>
    {
        using type = std::tuple<Second, First, Args...>;
    };

    template<typename Tuple>
    struct swap_first_two_tuple_types
        : swap_first_two_tuple_types_base<remove_type_qualifiers_t<Tuple>>
    {};

    template<typename Tuple>
    using swap_first_two_tuple_types_t =
        typename swap_first_two_tuple_types<Tuple>::type;
}
