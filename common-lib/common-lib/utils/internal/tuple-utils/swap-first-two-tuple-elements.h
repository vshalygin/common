#pragma once
#include <common-lib/mpl/tuple-traits.h>
#include <common-lib/mpl/type-transform.h>
#include <tuple>

namespace vshalygin::cl::internal {
    template<std::size_t... I, class Arg1, class Arg2, class... Args>
    std::tuple<Arg2, Arg1, Args...> swap_first_two_tuple_elements_impl2
                                           (std::tuple<Arg1, Arg2, Args...> &&t,
                                            std::index_sequence<I...>)
    {
        return { std::get<1>(std::move(t)),
                 std::get<0>(std::move(t)),
                 std::get<I + 2>(std::move(t))... };
    }

    template<class Arg1, class Arg2, class... Args>
    std::tuple<Arg2, Arg1, Args...> swap_first_two_tuple_elements_impl
                                            (std::tuple<Arg1, Arg2, Args...> t)
    {
        return swap_first_two_tuple_elements_impl2(std::move(t),
                                                   std::index_sequence_for<Args...>{});
    }

    template<typename Tuple>
    auto swap_first_two_tuple_elements (Tuple &&t)
    {
        static_assert(is_std_tuple_v<Tuple>,
                      "argument is not std::tuple");
        static_assert(!std::is_volatile_v<std::remove_reference_t<Tuple>>,
                      "volatile tuple is not supported");
        static_assert(std::tuple_size_v<remove_type_qualifiers_t<Tuple>> > 1,
                      "tuple size must be greater than 1");

        return swap_first_two_tuple_elements_impl(std::forward<Tuple>(t));
    }
}
