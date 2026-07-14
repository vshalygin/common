#pragma once
#include <common-lib/mpl/type-transform.h>

namespace vshalygin::cl {
    template<typename To, typename From>
    To type_qualifiers_cast(From &&from)
    {
        static_assert(std::is_same_v<remove_type_qualifiers_t<To>, remove_type_qualifiers_t<From>>,
                      "types are different");

        return static_cast<To>(from);
    }
}
