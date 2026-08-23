#pragma once

#include <common-lib/thread/thread-pool/thread-pool.h>

#include <memory>

namespace vshalygin::example {
    class app
    {
    public:
        app();

        app(const app &) = delete;
        app &operator=(const app &) = delete;

        int run() noexcept;

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;
    };
}
