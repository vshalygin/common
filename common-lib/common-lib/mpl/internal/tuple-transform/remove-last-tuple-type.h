#pragma once
#include "merge-tuples.h"
#include "../type-transform/remove-type-qualifiers.h"
#include <tuple>

namespace vshalygin::cl::internal {
    template<typename Tuple>
    struct remove_last_tuple_type_base
    {
        static_assert(sizeof(Tuple) == 0,
                      "bad tuple type");
    };

    template<typename Arg>
    struct remove_last_tuple_type_base<std::tuple<Arg>>
    {
        using type = std::tuple<>;
    };

    template<typename FirstArg, typename...Args>
    struct remove_last_tuple_type_base<std::tuple<FirstArg, Args...>>
        : public remove_last_tuple_type_base<std::tuple<Args...>>
    {
        using base_type = remove_last_tuple_type_base<std::tuple<Args...>>;

        using type = merge_tuples_t<std::tuple<FirstArg>, typename base_type::type>;
    };

    template<typename Tuple>
    struct remove_last_tuple_type
        : remove_last_tuple_type_base<remove_type_qualifiers_t<Tuple>>
    {};

    template<typename Tuple>
    using remove_last_tuple_type_t =
        typename remove_last_tuple_type<Tuple>::type;
}
