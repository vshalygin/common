#pragma once
#include <stdlib.h>

namespace vshalygin::cl::internal {
    template<typename Tuple>
    inline constexpr size_t tuple_size_v =
        std::tuple_size_v<remove_type_qualifiers_t<Tuple>>;
}
