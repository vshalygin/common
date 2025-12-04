#include "latch.h"
#include <chrono>
#include <mutex>
#include <condition_variable>

namespace vsh::cl {
    class latch::impl
    {
    public:
        impl(size_t count)
            : m_counter(count)
        {}

        impl(impl &) = delete;
        impl &operator=(impl &) = delete;

        void count_down()
        {
            bool need_notify = false;
            {
                std::unique_lock lock(m_mtx);
                if(m_counter && (--m_counter == 0)) {
                    need_notify = true;
                }
            }
            if(need_notify) {
                m_cv.notify_all();
            }
        }

        void wait()
        {
            std::unique_lock lock(m_mtx);
            m_cv.wait(lock, [this]() { return m_counter == 0; });
        }

        bool wait_for(const std::chrono::microseconds &microseconds)
        {
            std::unique_lock lock(m_mtx);
            return m_cv.wait_for(lock, microseconds, [this]() { return m_counter == 0; });
        }

    private:
        mutable std::mutex m_mtx;
        size_t m_counter;

        std::condition_variable m_cv;
    };

    latch::latch(size_t count)
        : m_impl(std::make_unique<impl>(count))
    {}

    latch::~latch() = default;

    void latch::count_down()
    {
        m_impl->count_down();
    }

    void latch::wait()
    {
        m_impl->wait();
    }

    bool latch::wait_for(const std::chrono::microseconds &microseconds)
    {
        return m_impl->wait_for(microseconds);
    }
}
