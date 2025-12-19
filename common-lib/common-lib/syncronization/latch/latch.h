#pragma once
#include <memory>
#include <chrono>

namespace vsh::cl {
    class latch final
    {
    public:
        explicit latch(size_t count);

        ~latch();

        latch(latch &) = delete;
        latch &operator=(latch &) = delete;

        latch(latch &&);
        latch &operator=(latch &&);

        void count_down();

        void wait();
        bool wait_for(const std::chrono::microseconds &microseconds);

    private:
        class impl;
        std::unique_ptr<impl> m_impl;
    };
}
