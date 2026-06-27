#pragma once
#include "internal/tuple-utils/swap-first-two-tuple-elements.h"
#include "internal/tuple-utils/merge-tuples.h"
#include "internal/tuple-utils/extract-first-tuple-element.h"
#include "internal/tuple-utils/extract-last-tuple-element.h"
#include "internal/tuple-utils/sort-tuple.h"
#include "internal/tuple-utils/forward-tuple-element.h"

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

    template<typename Comparator, typename Tuple>
    auto sort_tuple(Tuple &&t)
    {
        return internal::sort_tuple<Comparator>(std::forward<Tuple>(t));
    }

    template<size_t I, typename Tuple>
    constexpr decltype(auto) forward_tuple_element(Tuple &&tuple) noexcept
    {
        return internal::forward_tuple_element<I>(std::forward<Tuple>(tuple));
    }
}
