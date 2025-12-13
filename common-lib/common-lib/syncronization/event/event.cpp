#include "event.h"
#include "common-lib/syncronization/spinlock/spinlock-guard.h"
#include <condition_variable>
#include <atomic>

namespace vsh::cl {
    class event::impl final
    {
    public:
        explicit impl(bool manual_reset)
            : m_manual_reset(manual_reset)
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

        bool wait_for(const std::chrono::microseconds &mcs)
        {
            spinlock_guard guard(m_spinlock);
            const auto deadline = std::chrono::steady_clock::now() + mcs;
            auto signaled = m_cv.wait_until(guard, deadline, [this]() { return m_is_set; });
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

    event::event(bool manual_reset)
        : m_impl(std::make_unique<impl>(manual_reset))
    {}

    event::~event() = default;

    event::event(event &&) noexcept = default;
    event &event::operator=(event &&) noexcept = default;

    void event::set() noexcept
    {
        m_impl->set();
    }

    bool event::is_set() const noexcept
    {
        return m_impl->is_set();
    }

    void event::reset() noexcept
    {
        m_impl->reset();
    }

    void event::wait()
    {
        m_impl->wait();
    }

    bool event::wait_for(const std::chrono::microseconds &mcs)
    {
        return m_impl->wait_for(mcs);
    }
}
