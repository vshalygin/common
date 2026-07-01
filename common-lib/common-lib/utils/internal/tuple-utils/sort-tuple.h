#pragma once
#include "make-tuple-index-sequense.h"
#include "swap-first-two-tuple-elements.h"
#include "extract-first-tuple-element.h"
#include "merge-tuples.h"
#include "forward-tuple-element.h"

#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/type-traits.h>

namespace vshalygin::cl::internal {
    namespace sort_tuple_impl2 {
        template<typename RefT, typename T>
        struct tuple_element_ref
        {
            static_assert(std::is_reference_v<RefT>);

            using ref_type = RefT;
            using type = T;

            tuple_element_ref(ref_type r)
                : ref(std::forward<ref_type>(r))
            {}

            tuple_element_ref(const tuple_element_ref &other)
                : ref(std::forward<ref_type>(other.ref))
            {}

            ref_type ref;
        };

        template<std::size_t... I, typename Tuple>
        auto make_ref_tuple_impl(Tuple &&tuple,
                                 std::index_sequence<I...>)
        {
            using ret_t = std::tuple<tuple_element_ref<
                decltype(forward_tuple_element<I>(std::forward<Tuple>(tuple))),
                tuple_element_t<Tuple, I>>...>;

            return ret_t{ tuple_element_ref<
                                   decltype(forward_tuple_element<I>(std::forward<Tuple>(tuple))),
                                   tuple_element_t<Tuple, I>>
                                      (forward_tuple_element<I>(std::forward<Tuple>(tuple)))... };
        }

        template<typename Tuple>
        auto make_ref_tuple(Tuple &&tuple)
        {
            return make_ref_tuple_impl(std::forward<Tuple>(tuple),
                                       make_tuple_index_sequence(tuple));
        }

        template<std::size_t... I, typename RefTuple>
        auto restore_tuple(RefTuple sorted_ref_tuple,
                           std::index_sequence<I...>)
        {
            using ret_t = std::tuple<typename tuple_element_t<RefTuple, I>::type...>;

            return ret_t{ std::forward<typename tuple_element_t<RefTuple, I>::ref_type>
                                 (std::get<I>(sorted_ref_tuple).ref)... };
        }

        template<typename Comparator, typename RefTuple>
        auto do_bubble_rise(RefTuple ref_tuple)
        {
            if constexpr(tuple_size_v<RefTuple> == 1) {
                return ref_tuple;
            } else {
                constexpr bool comp_res = Comparator::template compare<
                    remove_type_qualifiers_t<typename std::tuple_element_t<0, RefTuple>::type>,
                    remove_type_qualifiers_t<typename std::tuple_element_t<1, RefTuple>::type>>();
                if constexpr(!comp_res) {
                    auto new_ref_tuple = do_swap_first_two_tuple_elements(ref_tuple);
                    auto split = do_extract_first_tuple_element(new_ref_tuple);
                    return do_merge_tuples(split.first, do_bubble_rise<Comparator>(split.second));
                }
                else {
                    auto split = do_extract_first_tuple_element(ref_tuple);
                    return do_merge_tuples(split.first, do_bubble_rise<Comparator>(split.second));
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
                auto split = do_extract_last_tuple_element(bubbled);
                return do_merge_tuples(sort_ref_tuple<Comparator>(split.second), split.first);
            }
        }
    }

    template<typename Comparator, typename Tuple>
    auto do_sort_tuple(Tuple &&tuple)
    {
        static_assert(is_std_tuple_v<Tuple>,
                      "type must be std::tuple");
        static_assert(!std::is_volatile_v<std::remove_reference_t<Tuple>>,
                      "volatile tuple is not supported");

        if constexpr(tuple_size_v<Tuple> == 0 || tuple_size_v<Tuple> == 1) {
            return std::forward<Tuple>(tuple);
        }

        auto idx_ref_tuple = sort_tuple_impl2::make_ref_tuple(std::forward<Tuple>(tuple));

        auto sorted = sort_tuple_impl2::sort_ref_tuple<Comparator>(idx_ref_tuple);

        return sort_tuple_impl2::restore_tuple(sorted,
                                               make_tuple_index_sequence(sorted));
    }
}
