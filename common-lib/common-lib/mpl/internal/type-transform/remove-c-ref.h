#pragma once
#include <type_traits>

namespace vshalygin::cl::internal {
    template<typename T>
    using remove_c_ref_t = std::remove_const_t<std::remove_reference_t<T>>;
}
