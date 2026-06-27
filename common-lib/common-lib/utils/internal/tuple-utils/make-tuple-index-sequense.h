#pragma once
#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/type-traits.h>

namespace vshalygin::cl::internal {
    template<typename Tuple>
    auto make_tuple_index_sequence(const Tuple &)
    {
        static_assert(is_std_tuple_v<Tuple>,
                      "argument is not std::tuple");

        return std::make_index_sequence<
            std::tuple_size_v<remove_type_qualifiers_t<Tuple>>>{};
    }
}
