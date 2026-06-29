#pragma once
#include <common-lib/mpl/tuple-traits.h>
#include <tuple>

namespace vshalygin::cl::internal {
    template<typename Tuple, typename F, size_t...I>
    void for_each_tuple_element_reverse_impl(Tuple &&tuple,
                                             F &&f,
                                             std::index_sequence<I...>)
    {
        (f(std::get<tuple_size_v<Tuple> -I - 1 >(std::forward<Tuple>(tuple))), ...);
    }

    template<typename Tuple, typename F>
    void for_each_tuple_element_reverse(Tuple &&tuple, F &&f)
    {
        static_assert(is_std_tuple_v<Tuple>,
                      "bad tuple type");

        for_each_tuple_element_reverse_impl(std::forward<Tuple>(tuple),
                                            std::forward<F>(f),
                                            std::make_index_sequence<tuple_size_v<Tuple>>());
    }
}
