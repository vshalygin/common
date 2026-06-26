#pragma once
#include <common-lib/mpl/function-traits.h>
#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/type-traits.h>

#include <tuple>

namespace vshalygin::cl::internal {
    template<std::size_t... I, typename Tuple>
    auto extract_last_tuple_element_impl(Tuple &&tuple,
                                         std::index_sequence<I...>)
    {
        using T = remove_type_qualifiers_t<Tuple>;
        using left_t = std::tuple_element_t<tuple_size_v<Tuple>-1, T>;
        using right_t = std::tuple<std::tuple_element_t<I, T>...>;
        using res_t = std::pair<left_t, right_t>;

        return res_t{ std::get<tuple_size_v<Tuple> - 1>(std::forward<Tuple>(tuple)),
                      right_t{ std::get<I>(std::forward<Tuple>(tuple))...} };
    }

    template<typename Tuple>
    auto extract_last_tuple_element(Tuple &&tuple)
    {
        static_assert(is_std_tuple_v<Tuple>,
                      "type must be std::tuple");
        static_assert(std::tuple_size_v<remove_type_qualifiers_t<Tuple>> > 0,
                      "tuple type cannot be empty");


        return extract_last_tuple_element_impl(
                                    std::forward<Tuple>(tuple),
                                    std::make_index_sequence<tuple_size_v<Tuple> - 1>{});
    }
}
