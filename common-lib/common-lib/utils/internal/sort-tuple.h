#pragma once
#include "make-tuple-index-sequense.h"
#include "swap-first-two-tuple-elements.h"
#include "extract-first-tuple-element.h"
#include "merge-tuples.h"

#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/type-traits.h>

namespace vshalygin::cl::internal {
    //TODO move to mpl
    template<typename Tuple, std::size_t I>
    using tuple_element_t = std::tuple_element_t<I, remove_type_qualifiers_t<Tuple>>;

    template<typename Tuple, std::size_t I>
    using tuple_element_ref_t =
        decltype(std::get<I>(std::declval<Tuple>()));

    template<typename T, typename RefT>
    struct tuple_element_ref
    {
        using type = T;
        using ref_type = RefT;

        tuple_element_ref(RefT r)
            : ref(std::forward<RefT>(r))
        {}

        tuple_element_ref(const tuple_element_ref &other)
            : ref(std::forward<RefT>(other.ref))
        {}

        RefT ref;
    };

    template<std::size_t... I, typename Tuple>
    auto make_ref_tuple_impl(Tuple &&tuple,
                             std::index_sequence<I...>)
    {
        using ret_t = std::tuple<tuple_element_ref<tuple_element_t<Tuple, I>,
                                                   tuple_element_ref_t<Tuple, I>>...>;

        return ret_t{ std::get<I>(std::forward<Tuple>(tuple))... };
    }

    template<typename Tuple>
    auto make_ref_tuple(Tuple &&tuple)
    {
        return make_ref_tuple_impl(std::forward<Tuple>(tuple),
                                   make_tuple_index_sequence(tuple));
    }

    template<typename Comparator, typename RefTuple>
    auto do_bubble_rise(RefTuple ref_tuple)
    {
        if constexpr (tuple_size_v<RefTuple> == 1) {
            return ref_tuple;
        } else {
            constexpr bool comp_res = Comparator::template compare<
                                        typename std::tuple_element_t<0, RefTuple>::type,
                                        typename std::tuple_element_t<1, RefTuple>::type>();
            if constexpr(!comp_res) {
                auto new_ref_tuple = swap_first_two_tuple_elements(ref_tuple);
                auto split = extract_first_tuple_element(new_ref_tuple);
                return merge_tuples(split.first, do_bubble_rise<Comparator>(split.second));
            } else {
                auto split = extract_first_tuple_element(ref_tuple);
                return merge_tuples(split.first, do_bubble_rise<Comparator>(split.second));
            }
        }
    }

    template<typename Comparator, typename RefTuple>
    auto sort_ref_tuple(RefTuple tuple)
    {
        if constexpr(tuple_size_v<RefTuple> == 1) {
            return tuple;
        } else {
            auto bubbled = do_bubble_rise<Comparator>(tuple);
            auto split = extract_last_tuple_element(bubbled);
            return merge_tuples(sort_ref_tuple<Comparator>(split.second), split.first);
        }
    }

    template<std::size_t... I, typename RefTuple>
    auto restore_sorted_tuple(RefTuple sorted_ref_tuple,
                              std::index_sequence<I...>)
    {
        using RT = remove_type_qualifiers_t<RefTuple>;
        using ret_t = std::tuple<typename std::tuple_element_t<I, RT>::type...>;
        
        return ret_t{ static_cast<typename std::tuple_element_t<I, RT>::type>
                             (std::get<I>(sorted_ref_tuple).ref)... };
    }

    template<typename Comparator, typename Tuple>
    auto sort_tuple(Tuple &&tuple)
    {
        static_assert(is_std_tuple_v<Tuple>,
                      "type must be std::tuple");
        static_assert(!std::is_volatile_v<std::remove_reference_t<Tuple>>,
                      "volatile tuple is not supported");

        if constexpr(tuple_size_v<Tuple> == 0 || tuple_size_v<Tuple> == 1) {
            return std::forward<Tuple>(tuple);
        }

        auto idx_ref_tuple = make_ref_tuple(std::forward<Tuple>(tuple));

        auto sorted = sort_ref_tuple<Comparator>(idx_ref_tuple);

        return restore_sorted_tuple(sorted,
                                    make_tuple_index_sequence(sorted));
    }
}
