#pragma once
#include <type_traits>

namespace vshalygin::cl::internal {
    template<typename T>
    using add_const_t =
        std::conditional_t<
            std::is_lvalue_reference_v<T>,
            std::add_lvalue_reference_t<
                  std::add_const_t<std::remove_reference_t<T>>>,
        std::conditional_t<
            std::is_rvalue_reference_v<T>,
            std::add_rvalue_reference_t<
                std::add_const_t<std::remove_reference_t<T>>>,
        std::add_const_t<T>>>;
}
