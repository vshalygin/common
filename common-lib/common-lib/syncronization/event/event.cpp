#include "event.h"
#include <condition_variable>
#include <mutex>

namespace vsh::cl {
    class event::impl final
    {
    public:
        impl() = default;

        impl(impl &) = delete;
        impl &operator=(impl &) = delete;

        void set()
        {
            {
                std::lock_guard guard(m_mtx);
                m_is_set = true;
            }

            m_cv.notify_all();
        }

        bool is_set() const
        {
            std::lock_guard guard(m_mtx);
            return m_is_set;
        }

        void clear()
        {
            std::lock_guard guard(m_mtx);
            m_is_set = false;
        }

        void wait()
        {
            std::unique_lock lock(m_mtx);
            m_cv.wait(lock, [this]() { return m_is_set; });
        }

        bool wait_for(const std::chrono::microseconds &mcs)
        {
            std::unique_lock lock(m_mtx);
            return m_cv.wait_for(lock, mcs, [this]() { return m_is_set; });
        }

    private:
        mutable std::mutex m_mtx;
        bool m_is_set = false;

        std::condition_variable m_cv;
    };

    event::event()
        : m_impl(std::make_unique<impl>())
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

    void event::clear()
    {
        m_impl->clear();
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