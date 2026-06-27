#pragma once
#include "../type-transform/remove-type-qualifiers.h"
#include <tuple>

namespace vshalygin::cl::internal {
    template<typename Tuple, std::size_t I>
    using tuple_element_t = std::tuple_element_t<I, remove_type_qualifiers_t<Tuple>>;
}
