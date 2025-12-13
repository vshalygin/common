#pragma once
#include "spinlock.h"

namespace vsh::cl {
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
