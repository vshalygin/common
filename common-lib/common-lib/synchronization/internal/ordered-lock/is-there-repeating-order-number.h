#pragma once
#include "order-comparator.h"

#include <common-lib/mpl/tuple-traits.h>
#include <common-lib/mpl/tuple-transform.h>
#include <common-lib/mpl/type-traits.h>

#include <type_traits>
#include <tuple>

namespace vshalygin::cl::internal {
    namespace ordered_lock_impl {
        template<typename OrderedTuple, typename IndexSeq>
        struct is_there_repeating_order_number_base;

        template<typename OrderedTuple, size_t...I>
        struct is_there_repeating_order_number_base<OrderedTuple, std::index_sequence<I...>>
        {
            static constexpr bool value =
              ((remove_type_qualifiers_t<tuple_element_t<OrderedTuple, I>>::order ==
               remove_type_qualifiers_t<tuple_element_t<OrderedTuple, I + 1>>::order) || ...);
        };

        template<typename...OrderedLockable>
        struct is_there_repeating_order_number
            : is_there_repeating_order_number_base<
                 sort_tuple_t<std::tuple<OrderedLockable...>, order_comparator>,
                 std::make_index_sequence<sizeof...(OrderedLockable) - 1>>
        {};

        template<typename OrderedLockable>
        struct is_there_repeating_order_number<OrderedLockable>
        {
            static constexpr bool value = false;
        };

        template<>
        struct is_there_repeating_order_number<>
        {
            static constexpr bool value = false;
        };
    }

    template<typename...OrderedLockable>
    constexpr bool is_there_repeating_order_number_v =
        ordered_lock_impl::is_there_repeating_order_number<OrderedLockable...>::value;
}
