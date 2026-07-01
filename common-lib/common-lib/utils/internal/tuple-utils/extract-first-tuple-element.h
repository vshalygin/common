#pragma once
#include <common-lib/mpl/function-traits.h>
#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/type-traits.h>

#include <tuple>

namespace vshalygin::cl::internal {
    namespace extract_first_tuple_element_impl {
        template<std::size_t... I, typename Tuple>
        auto extract_first_tuple_element(Tuple &&tuple,
                                         std::index_sequence<I...>)
        {
            using T = remove_type_qualifiers_t<Tuple>;
            using left_t = std::tuple<std::tuple_element_t<0, T>>;
            using right_t = std::tuple<std::tuple_element_t<I + 1, T>...>;
            using res_t = std::pair<left_t, right_t>;

            return res_t{ left_t { std::get<0>(std::forward<Tuple>(tuple)) } ,
                          right_t{ std::get<I + 1>(std::forward<Tuple>(tuple))...} };
        }
    }
    
    template<typename Tuple>
    auto do_extract_first_tuple_element(Tuple &&tuple)
    {
        static_assert(is_std_tuple_v<Tuple>,
                      "type must be std::tuple");
        static_assert(std::tuple_size_v<remove_type_qualifiers_t<Tuple>> > 0,
                      "tuple type cannot be empty");
        static_assert(!std::is_volatile_v<std::remove_reference_t<Tuple>>,
                      "volatile tuple is not supported");

        return extract_first_tuple_element_impl::extract_first_tuple_element(
                                std::forward<Tuple>(tuple),
                                std::make_index_sequence<tuple_size_v<Tuple> - 1>{});
    }
}
