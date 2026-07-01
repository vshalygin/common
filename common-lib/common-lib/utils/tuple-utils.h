#pragma once
#include "internal/tuple-utils/swap-first-two-tuple-elements.h"
#include "internal/tuple-utils/merge-tuples.h"
#include "internal/tuple-utils/extract-first-tuple-element.h"
#include "internal/tuple-utils/extract-last-tuple-element.h"
#include "internal/tuple-utils/sort-tuple.h"
#include "internal/tuple-utils/sort-tuple2.h"
#include "internal/tuple-utils/forward-tuple-element.h"
#include "internal/tuple-utils/for-each-tuple-element.h"
#include "internal/tuple-utils/for-each-tuple-element-reverse.h"
#include "internal/tuple-utils/tie-tuple.h"

namespace vshalygin::cl {
    template<typename Tuple>
    auto swap_first_two_tuple_elements(Tuple &&t)
    {
        return internal::do_swap_first_two_tuple_elements(std::forward<Tuple>(t));
    }

    template<typename...Tuples>
    auto merge_tuples(Tuples&&...tuples)
    {
        return internal::do_merge_tuples(std::forward<Tuples>(tuples)...);
    }

    template<typename Tuple>
    auto extract_first_tuple_element(Tuple &&tuple)
    {
        return internal::do_extract_first_tuple_element(std::forward<Tuple>(tuple));
    }

    template<typename Tuple>
    auto extract_last_tuple_element(Tuple &&tuple)
    {
        return internal::do_extract_last_tuple_element(std::forward<Tuple>(tuple));
    }

    template<typename Comparator, typename Tuple>
    sort_tuple_t<Tuple, Comparator> sort_tuple(Tuple &&t)
    {
        return internal::do_sort_tuple<Comparator>(std::forward<Tuple>(t));
    }

    template<typename Comparator, typename Tuple>
    sort_tuple_t<Tuple, Comparator> sort_tuple2(Tuple &&t)
    {
        return internal::do_sort_tuple2<Comparator>(std::forward<Tuple>(t));
    }

    template<size_t I, typename Tuple>
    constexpr decltype(auto) forward_tuple_element(Tuple &&tuple) noexcept
    {
        return internal::do_forward_tuple_element<I>(std::forward<Tuple>(tuple));
    }

    template<typename Tuple, typename F>
    void for_each_tuple_element(Tuple &&tuple, F &&f)
    {
        internal::do_for_each_tuple_element(std::forward<Tuple>(tuple),
                                            std::forward<F>(f));
    }

    template<typename Tuple, typename F>
    void for_each_tuple_element_reverse(Tuple &&tuple, F &&f)
    {
        internal::do_for_each_tuple_element_reverse(std::forward<Tuple>(tuple),
                                                    std::forward<F>(f));
    }

    template<typename... Ts>
    auto tie_tuple(std::tuple<Ts...> &t)
    {
        return internal::do_tie_tuple(t);
    }
}
