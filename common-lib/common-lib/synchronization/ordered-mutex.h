#pragma once
#include <mutex>

namespace vshalygin::cl {
    template<size_t Order>
    class ordered_mutex
    {
    public:
        static constexpr size_t order = Order;

        ordered_mutex() = default;

        ordered_mutex(const ordered_mutex &) = delete;
        ordered_mutex &operator=(const ordered_mutex &) = delete;

        void lock()
        {
            m_mtx.lock();
        }

        void unlock() noexcept
        {
            m_mtx.unlock();
        }

        std::mutex &get_underlying() noexcept
        {
            return m_mtx;
        }

        const std::mutex &get_underlying() const noexcept
        {
            return m_mtx;
        }

    private:
        std::mutex m_mtx;
    };
}
