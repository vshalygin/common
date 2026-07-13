#pragma once
#include <memory>
#include <mutex>
#include <type_traits>

namespace vshalygin::cl::internal {
    template<typename T, typename Enable = void>
    class safe_ptr_proxy
    {
    public:
        safe_ptr_proxy(std::unique_lock<std::recursive_mutex> lock,
                       T *val) noexcept
            : m_lock(std::move(lock))
            , m_val(val)
        {}

        safe_ptr_proxy(const safe_ptr_proxy &) = delete;
        safe_ptr_proxy &operator=(const safe_ptr_proxy &) = delete;

        T *operator->() noexcept
        {
            return m_val;
        }

    private:
        std::unique_lock<std::recursive_mutex> m_lock;
        T *m_val;
    };

    template<typename T, typename Enable = void>
    class safe_ptr_controller
    {
    public:
        safe_ptr_controller() = default;
        explicit safe_ptr_controller(T *val)
            : m_value(val)
        {}

        safe_ptr_controller(const safe_ptr_controller &) = delete;
        safe_ptr_controller &operator=(const safe_ptr_controller &) = delete;

        ~safe_ptr_controller()
        {
            delete m_value;
        }

        auto create_proxy()
        {
            std::unique_lock l(m_mtx);
            return safe_ptr_proxy<T>(std::move(l), m_value);
        }

        void lock()
        {
            m_mtx.lock();
        }

        void unlock() noexcept
        {
            m_mtx.unlock();
        }

    private:
        std::recursive_mutex m_mtx;
        T *m_value = nullptr;
    };
}
