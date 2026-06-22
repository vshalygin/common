#pragma once
#include <type_traits>

namespace vshalygin::cl::internal {
    template<typename T>
    using remove_type_qualifiers_t = std::remove_cv_t<std::remove_reference_t<T>>;
}
