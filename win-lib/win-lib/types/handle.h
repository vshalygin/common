#pragma once
#ifdef _WIN32
#include "internal/unique-handle.h"
#include "internal/handle-traits.h"

namespace vshalygin::win {
    using event_handle =
        internal::unique_handle<internal::event_handle_traits>;
}
#endif
