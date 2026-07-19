#include "../../event.h"

namespace vshalygin::cl {
    class event::impl final
    {
    public:
        impl(bool manual_reset,
             bool initial_set)
            : m_manual_reset(manual_reset)
            , m_is_set(initial_set)
        {}

        impl(impl &) = delete;
        impl &operator=(impl &) = delete;

        void set() noexcept
        {
            spinlock_guard guard(m_spinlock);
            m_is_set = true;

            guard.unlock();
            if(m_manual_reset) {
                m_cv.notify_all();
            } else {
                m_cv.notify_one();
            }
        }

        bool is_set() const noexcept
        {
            spinlock_guard guard(m_spinlock);
            return m_is_set;
        }

        void reset() noexcept
        {
            spinlock_guard guard(m_spinlock);
            m_is_set = false;
        }

        void wait()
        {
            spinlock_guard guard(m_spinlock);
            m_cv.wait(guard, [this]() { return m_is_set; });

            if(!m_manual_reset) {
                m_is_set = false;
            }
        }

        bool wait_for(std::chrono::milliseconds timeout)
        {
            using clock = std::chrono::steady_clock;

            const auto now = clock::now();
            const auto max = clock::time_point::max();

            const clock::time_point tp = (timeout > max - now) ? max : now + timeout;

            spinlock_guard guard(m_spinlock);
            auto signaled = m_cv.wait_until(guard, tp, [this]() { return m_is_set; });
            if(signaled && !m_manual_reset) {
                m_is_set = false;
            }

            return signaled;
        }

    private:
        const bool m_manual_reset;
        bool m_is_set = false;

        mutable spinlock m_spinlock;
        std::condition_variable_any m_cv;
    };

    event::event(bool manual_reset,
                 bool initial_set)
        : m_impl(std::make_shared<impl>(manual_reset, initial_set))
    {}

    void event::set() noexcept
    {
        std::shared_ptr(m_impl)->set();
    }

    bool event::is_set() const noexcept
    {
        return std::shared_ptr(m_impl)->is_set();
    }

    void event::reset() noexcept
    {
        std::shared_ptr(m_impl)->reset();
    }

    void event::wait()
    {
        std::shared_ptr(m_impl)->wait();
    }

    bool event::wait_for(std::chrono::milliseconds timeout)
    {
        return std::shared_ptr(m_impl)->wait_for(timeout);
    }
}
