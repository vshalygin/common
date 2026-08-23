#include "app.h"

namespace vshalygin::example {
    app::app()
        : m_thread_pool(std::make_shared<cl::thread_pool>(2))
    {}

    int app::run() noexcept
    {
        try {
            return 0;
        } catch (...) {
        }

        return 1;
    }
}
