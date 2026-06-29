#pragma once
#include <type_traits>

namespace vshalygin::cl::internal {
    struct order_comparator
    {
        template<typename T, typename U>
        static constexpr bool compare()
        {
            return std::remove_pointer_t<T>::order < std::remove_pointer_t<U>::order;
        }
    };
}
