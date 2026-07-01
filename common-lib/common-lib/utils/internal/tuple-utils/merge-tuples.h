#pragma once
#include "make-tuple-index-sequense.h"
#include <common-lib/mpl/tuple-traits.h>
#include <common-lib/mpl/type-transform.h>

#include <tuple>

namespace vshalygin::cl::internal {
    namespace merge_tuples_impl2 {
        template<std::size_t... I1, typename Tuple1, std::size_t... I2, typename Tuple2>
        auto merge_two_tuples_impl(Tuple1 &&t1,
                                   std::index_sequence<I1...>,
                                   Tuple2 &&t2,
                                   std::index_sequence<I2...>)
        {
            using T1 = remove_type_qualifiers_t<Tuple1>;
            using T2 = remove_type_qualifiers_t<Tuple2>;
            using res_t = std::tuple<std::tuple_element_t<I1, T1>...,
                std::tuple_element_t<I2, T2>...>;
            return res_t{ std::get<I1>(std::forward<Tuple1>(t1))...,
                          std::get<I2>(std::forward<Tuple2>(t2))... };
        }

        template<typename Tuple1, typename Tuple2>
        auto merge_two_tuples(Tuple1 &&t1, Tuple2 &&t2)
        {
            static_assert(is_std_tuple_v<Tuple1> && is_std_tuple_v<Tuple2>,
                          "types must be std::tuple");

            return merge_two_tuples_impl(std::forward<Tuple1>(t1),
                                         make_tuple_index_sequence(t1),
                                         std::forward<Tuple2>(t2),
                                         make_tuple_index_sequence(t2));
        }
    }
   

    template<typename Tuple>
    auto do_merge_tuples(Tuple &&tuple)
    {
        static_assert(is_std_tuple_v<Tuple>,
                      "type must be std::tuple");

        return std::forward<Tuple>(tuple);
    }

    template<typename FirstTuple, typename SecondTuple, typename...RestTuples>
    auto do_merge_tuples(FirstTuple &&first_tuple,
                         SecondTuple &&second_tuple,
                         RestTuples &&...rest_tuples)
    {
        static_assert(is_std_tuple_v<FirstTuple> &&
                      is_std_tuple_v<SecondTuple> &&
                      (is_std_tuple_v<RestTuples> && ...),
                      "types must be std::tuple");
        static_assert(!std::is_volatile_v<std::remove_reference_t<FirstTuple>> &&
                      !std::is_volatile_v<std::remove_reference_t<SecondTuple>> &&
                      !(std::is_volatile_v<std::remove_reference_t<RestTuples>> || ...),
                      "volatile tuples are not supported");

        auto merged = merge_tuples_impl2::merge_two_tuples(
                                    std::forward<FirstTuple>(first_tuple),
                                    std::forward<SecondTuple>(second_tuple));


        return do_merge_tuples(std::move(merged),
                               std::forward<RestTuples>(rest_tuples)...);
    }
}
