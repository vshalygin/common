#pragma once
#include "internal/tuple-transform/swap-first-two-tuple-types.h"
#include "internal/tuple-transform/merge-tuples.h"
#include "internal/tuple-transform/sort-tuple.h"
#include "internal/tuple-transform/last-tuple-type.h"
#include "internal/tuple-transform/remove-last-tuple-type.h"

namespace vshalygin::cl {
    template<typename Tuple>
    using swap_first_two_tuple_types_t =
        internal::swap_first_two_tuple_types_t<Tuple>;

    template<typename Tuple1, typename Tuple2>
    using merge_tuples_t =
        internal::merge_tuples_t<Tuple1, Tuple2>;

    template<typename Tuple>
    using last_tuple_type_t =
        internal::last_tuple_type_t<Tuple>;

    template<typename Tuple>
    using remove_last_tuple_type_t =
        internal::remove_last_tuple_type_t<Tuple>;

    template<typename Tuple, typename Comparator>
    using sort_tuple_t =
        internal::sort_tuple_t<Tuple, Comparator>;
}
