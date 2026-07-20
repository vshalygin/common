#pragma once
#include <atomic>
#include <thread>

namespace vshalygin::cl {
    class spinlock_guard;
    class spinlock;

    class spinlock final
    {
    public:
        spinlock() noexcept = default;

        spinlock(spinlock &) = delete;
        spinlock &operator=(spinlock &) = delete;

        void lock() noexcept
        {
            while(m_flag.test_and_set(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
        }
        void unlock() noexcept
        {
            m_flag.clear(std::memory_order_release);
        }

    private:
        std::atomic_flag m_flag = ATOMIC_FLAG_INIT;
    };

    class spinlock_guard final
    {
    public:
        spinlock_guard(spinlock &sl) noexcept
            : m_sl(sl)
            , m_owns(false)
        {
            lock();
        }

        ~spinlock_guard() noexcept
        {
            if(m_owns) {
                unlock();
            }
        }

        spinlock_guard(spinlock_guard &) = delete;
        spinlock_guard &operator=(spinlock_guard &) = delete;

        void lock() noexcept
        {
            m_sl.lock();
            m_owns = true;
        }

        void unlock() noexcept
        {
            m_sl.unlock();
            m_owns = false;
        }

    private:
        spinlock &m_sl;
        bool m_owns;
    };
}
