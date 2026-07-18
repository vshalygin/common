#pragma once
#include <mutex>

namespace vshalygin::cl {
    template<size_t Order>
    class ordered_mutex_ref
    {
    public:
        static constexpr size_t order = Order;

        ordered_mutex_ref() = default;
        explicit ordered_mutex_ref(std::mutex &mtx)
            : m_mtx(&mtx)
        {}

        ordered_mutex_ref(const ordered_mutex_ref &) = default;
        ordered_mutex_ref &operator=(const ordered_mutex_ref &) = default;

        ordered_mutex_ref(ordered_mutex_ref &&) = default;
        ordered_mutex_ref &operator=(ordered_mutex_ref &&) = default;

        void lock()
        {
            if(m_mtx) {
                m_mtx->lock();
            }
        }

        void unlock() noexcept
        {
            if(m_mtx) {
                m_mtx->unlock();
            }
        }

        bool has_underlying() const noexcept
        {
            return m_mtx != nullptr;
        }

        std::mutex &get_underlying() noexcept
        {
            return *m_mtx;
        }

        const std::mutex &get_underlying() const noexcept
        {
            return *m_mtx;
        }

    private:
        std::mutex *m_mtx = nullptr;
    };
}
