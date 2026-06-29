#pragma once
#include "merge-tuples.h"
#include "last-tuple-type.h"
#include "remove-last-tuple-type.h"

#include "../type-transform/remove-type-qualifiers.h"

#include <tuple>

namespace vshalygin::cl::internal {
    namespace sort_tuple_impl {
        template<typename Tuple, typename Comparator>
        struct do_bubble_rise
        {
            static_assert(sizeof(Tuple) == 0,
                          "bad tuple or comparator");
        };
        
        template<typename Comparator>
        struct do_bubble_rise<std::tuple<>, Comparator>
        {
            using type = std::tuple<>;
        };
        
        template<typename Arg, typename Comparator>
        struct do_bubble_rise<std::tuple<Arg>, Comparator>
        {
            using type = std::tuple<Arg>;
        };
        
        template<typename Arg1, typename Arg2, typename...Args, typename Comparator>
        struct do_bubble_rise<std::tuple<Arg1, Arg2, Args...>, Comparator>
        {
            static constexpr bool compare_res =
                Comparator::template compare<
                     remove_type_qualifiers_t<Arg1>,
                     remove_type_qualifiers_t<Arg2>>();

            using type = std::conditional_t<
                compare_res,
                merge_tuples_t<std::tuple<Arg1>,
                               typename do_bubble_rise<std::tuple<Arg2, Args...>,
                                   Comparator>::type>,
                merge_tuples_t<std::tuple<Arg2>,
                               typename do_bubble_rise<std::tuple<Arg1, Args...>,
                                   Comparator>::type>>;
        };
        
        template<typename Tuple, typename Comparator>
        struct sort_tuple_base
        {
            static_assert(sizeof(Tuple) == 0,
                          "bad tuple type");
        };

        template<typename Comparator>
        struct sort_tuple_base<std::tuple<>, Comparator>
        {
            using type = std::tuple<>;
        };

        template<typename...Args, typename Comparator>
        struct sort_tuple_base<std::tuple<Args...>, Comparator>
        {
            using tuple = std::tuple<Args...>;
            using bubbled_tuple = typename do_bubble_rise<tuple, Comparator>::type;
            using max_type = last_tuple_type_t<bubbled_tuple>;
            using rest_tuple_to_sort = remove_last_tuple_type_t<bubbled_tuple>;

            using type = merge_tuples_t<
                typename sort_tuple_base<rest_tuple_to_sort, Comparator>::type,
                std::tuple<max_type>>;
        };

        template<typename Tuple, typename Comparator>
        struct sort_tuple
            : sort_tuple_base<remove_type_qualifiers_t<Tuple>, Comparator>
        {};
    }
    
    template<typename Tuple, typename Comparator>
    using sort_tuple_t =
        typename sort_tuple_impl::sort_tuple<Tuple, Comparator>::type;
}
