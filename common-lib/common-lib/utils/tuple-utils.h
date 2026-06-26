#pragma once
#include "internal/swap-first-two-tuple-elements.h"
#include "internal/merge-tuples.h"

namespace vshalygin::cl {
    template<typename Tuple>
    auto swap_first_two_tuple_elements(Tuple &&t)
    {
        return internal::swap_first_two_tuple_elements(std::forward<Tuple>(t));
    }

    template<typename...Tuples>
    auto merge_tuples(Tuples&&...tuples)
    {
        return internal::merge_tuples(std::forward<Tuples>(tuples)...);
    }
}
