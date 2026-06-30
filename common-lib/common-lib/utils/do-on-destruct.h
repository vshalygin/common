#pragma once
#include "internal/do-on-destruct/do-on-destruct.h"

namespace vshalygin::cl {
    template<typename Func>
    using do_on_destruct =
        internal::do_on_destruct<Func>;
}
