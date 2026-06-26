#pragma once
#include "merge-tuples.h"

namespace vshalygin::cl::internal {
    template<typename Tuple, typename U>
    auto add_first_tuple_element(Tuple &&tuple, U &&value)
    {
        static_assert(is_std_tuple_v<Tuple>,
                      "type must be std::tuple");

        return merge_two_tuples(std::tuple{ std::forward<U>(value) },
                                std::forward<Tuple>(tuple));
    }
}
