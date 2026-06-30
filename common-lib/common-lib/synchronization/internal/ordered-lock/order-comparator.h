#pragma once
#include <common-lib/mpl/type-transform.h>
#include <type_traits>

namespace vshalygin::cl::internal {
    struct order_comparator
    {
        template<typename T, typename U>
        static constexpr bool compare()
        {
            return std::remove_pointer_t<remove_type_qualifiers_t<T>>::order <
                   std::remove_pointer_t<remove_type_qualifiers_t<U>>::order;
        }
    };
}
