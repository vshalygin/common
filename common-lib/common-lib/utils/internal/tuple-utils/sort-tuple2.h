#pragma once
#include "forward-tuple-element.h"
#include <common-lib/mpl/tuple-transform.h>
#include <common-lib/mpl/tuple-traits.h>

namespace vshalygin::cl::internal {
    namespace sort_tuple2_impl {
        template<typename T, size_t Idx>
        struct indexed_tuple_type
        {
            static constexpr size_t index = Idx;

            using type = T;
        };

        template<typename Comparator>
        struct indexed_tuple_type_comparator
        {
            template<typename T, typename U>
            static constexpr bool compare()
            {
                return Comparator::template compare<
                    remove_type_qualifiers_t<typename remove_type_qualifiers_t<T>::type>,
                    remove_type_qualifiers_t<typename remove_type_qualifiers_t<U>::type>>();
            }
        };

        template<typename Tuple, typename IndexSequence>
        struct make_indexed_tuple
        {
            static_assert(sizeof(Tuple) == 0);
        };

        template<size_t...I, typename Tuple>
        struct make_indexed_tuple<Tuple, std::index_sequence<I...>>
        {
            using type = std::tuple<indexed_tuple_type<tuple_element_t<Tuple, I>, I>...>;
        };

        template<typename Tuple>
        using make_indexed_tuple_t =
            typename make_indexed_tuple<
                         Tuple,
                         std::make_index_sequence<tuple_size_v<Tuple>>>::type;

        template<typename IndexedTuple, typename InitTuple, size_t...I>
        auto sort_tuple2(InitTuple &&tuple, std::index_sequence<I...>)
        {
            using ret_t = std::tuple<typename tuple_element_t<IndexedTuple, I>::type...>;

            return ret_t{ do_forward_tuple_element<tuple_element_t<IndexedTuple, I>::index>
                               (std::forward<InitTuple>(tuple))...};
        }
    }

    template<typename Comparator, typename Tuple>
    auto do_sort_tuple2(Tuple &&tuple)
    {
        static_assert(is_std_tuple_v<Tuple>,
                      "bad tuple");
        static_assert(!std::is_volatile_v<std::remove_reference_t<Tuple>>,
                      "volatile tuple is not supported");

        using indexed_tuple_t = sort_tuple2_impl::make_indexed_tuple_t
                                                    <remove_type_qualifiers_t<Tuple>>;
        using sorted_indexed_tuple_t =
            sort_tuple_t<indexed_tuple_t,
                         sort_tuple2_impl::indexed_tuple_type_comparator<Comparator>>;

        return sort_tuple2_impl::sort_tuple2<sorted_indexed_tuple_t>(
                               std::forward<Tuple>(tuple),
                               std::make_index_sequence<tuple_size_v<Tuple>>());
    }
}
