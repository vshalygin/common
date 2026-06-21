#include "spinlock.h"
#include <thread>

namespace vshalygin::cl {
    void spinlock::lock() noexcept
    {
        while(m_flag.test_and_set(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
    }

    void spinlock::unlock() noexcept
    {
        m_flag.clear(std::memory_order_release);
    }
}
