#pragma once
#ifdef _WIN32
#include "internal/unique-handle.h"
#include "internal/handle-traits.h"

namespace vshalygin::win {
    using event_handle =
        internal::unique_handle<internal::event_handle_traits>;

    using iocp_handle =
        internal::unique_handle<internal::iocp_handle_traits>;

    using pipe_handle =
        internal::unique_handle<internal::pipe_handle_traits>;
}

#endif
