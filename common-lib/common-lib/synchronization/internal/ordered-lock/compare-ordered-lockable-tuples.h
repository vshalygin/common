#pragma once
#include "order-comparator.h"
#include <common-lib/mpl/tuple-transform.h>
#include <common-lib/mpl/tuple-traits.h>
#include <tuple>

namespace vshalygin::cl::internal {
    namespace ordered_lock_impl {
        template<typename Tuple1, typename Tuple2>
        struct compare_ordered_lockable_tuples
        {
            using sorted_tuple1 = sort_tuple_t<Tuple1, order_comparator>;
            using sorted_tuple2 = sort_tuple_t<Tuple2, order_comparator>;
            using last_tuple1_type =
                tuple_element_t<sorted_tuple1, tuple_size_v<sorted_tuple1> - 1>;
            using first_tuple2_type =
                tuple_element_t<sorted_tuple2, 0>;

            static constexpr bool value =
                order_comparator::template compare<last_tuple1_type, first_tuple2_type>();
        };
    }
    
    template<typename Tuple1, typename Tuple2>
    constexpr bool compare_ordered_lockable_tuples_v =
        ordered_lock_impl::compare_ordered_lockable_tuples<Tuple1, Tuple2>::value;
}
