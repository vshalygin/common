#pragma once
#include <common-lib/mpl/tuple-traits.h>
#include <tuple>

namespace vshalygin::cl::internal {
    template<typename Tuple, typename F>
    void for_each_tuple_element(Tuple &&tuple, F &&f)
    {
        static_assert(is_std_tuple_v<Tuple>,
                      "bad tuple type");

        std::apply([&](auto&&... args) {
                        (f(std::forward<decltype(args)>(args)), ...);
                   }, std::forward<Tuple>(tuple));
    }
}
