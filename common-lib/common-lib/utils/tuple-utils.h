#pragma once
#include "internal/swap-first-two-tuple-elements.h"

namespace vshalygin::cl {
    template<typename Tuple>
    auto swap_first_two_tuple_elements(Tuple &&t)
    {
        return internal::swap_first_two_tuple_elements(std::forward<Tuple>(t));
    }
}
