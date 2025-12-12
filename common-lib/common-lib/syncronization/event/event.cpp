#include "event.h"
#include <condition_variable>
#include <mutex>

namespace vsh::cl {
    class event::impl final
    {
    public:
        explicit impl(bool manual_reset)
            : m_manual_reset(manual_reset)
            , m_is_set(std::make_shared<bool>(false))
        {}

        impl(impl &) = delete;
        impl &operator=(impl &) = delete;

        void set()
        {
            {
                std::lock_guard guard(m_mtx);
                *m_is_set = true;
                if(!m_manual_reset) {
                    m_is_set = std::make_shared<bool>(false);
                }
            }

            m_cv.notify_all();
        }

        bool is_set() const
        {
            std::lock_guard guard(m_mtx);
            return *m_is_set;
        }

        void reset()
        {
            std::lock_guard guard(m_mtx);
            *m_is_set = false;
        }

        void wait()
        {
            std::unique_lock lock(m_mtx);
            m_cv.wait(lock, [is_set = m_is_set]() { return *is_set; });
        }

        bool wait_for(const std::chrono::microseconds &mcs)
        {
            std::unique_lock lock(m_mtx);
            return m_cv.wait_for(lock, mcs, [is_set = m_is_set]() { return *is_set; });
        }

    private:
        const bool m_manual_reset;
        mutable std::mutex m_mtx;
        std::shared_ptr<bool> m_is_set;

        std::condition_variable m_cv;
    };

    event::event(bool manual_reset)
        : m_impl(std::make_unique<impl>(manual_reset))
    {}

    event::~event() = default;

    event::event(event &&) = default;
    event &event::operator=(event &&) = default;

    void event::set()
    {
        m_impl->set();
    }

    bool event::is_set() const
    {
        return m_impl->is_set();
    }

    void event::reset()
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