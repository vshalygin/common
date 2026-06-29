#pragma once
#include "../type-transform/remove-type-qualifiers.h"
#include "../tuple-traits/is-std-tuple.h"
#include "../tuple-traits/tuple-size.h"

namespace vshalygin::cl::internal {
    template<typename Tuple>
    struct last_tuple_type_base
    {
        static_assert(sizeof(Tuple) == 0,
                      "bad tuple type");
    };
    
    template<typename Arg>
    struct last_tuple_type_base<std::tuple<Arg>>
    {
        using type = Arg;
    };

    template<typename Arg, typename...Args>
    struct last_tuple_type_base<std::tuple<Arg, Args...>>
        : last_tuple_type_base<std::tuple<Args...>>
    {};

    template<typename Tuple>
    struct last_tuple_type
        : last_tuple_type_base<remove_type_qualifiers_t<Tuple>>
    {};

    template<typename Tuple>
    using last_tuple_type_t =
        typename last_tuple_type<Tuple>::type;
}
