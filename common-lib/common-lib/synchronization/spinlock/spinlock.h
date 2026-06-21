#pragma once
#include <atomic>

namespace vshalygin::cl {
    class spinlock final
    {
    public:
        spinlock() noexcept = default;

        spinlock(spinlock &) = delete;
        spinlock &operator=(spinlock &) = delete;

        void lock() noexcept;
        void unlock() noexcept;

    private:
        std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
    };
}
