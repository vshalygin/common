#pragma once
#include "internal/swap-first-two-tuple-elements.h"
#include "internal/merge-tuples.h"
#include "internal/extract-first-tuple-element.h"
#include "internal/extract-last-tuple-element.h"

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

    template<typename Tuple>
    auto extract_first_tuple_element(Tuple &&tuple)
    {
        return internal::extract_first_tuple_element(std::forward<Tuple>(tuple));
    }

    template<typename Tuple>
    auto extract_last_tuple_element(Tuple &&tuple)
    {
        return internal::extract_last_tuple_element(std::forward<Tuple>(tuple));
    }
}
