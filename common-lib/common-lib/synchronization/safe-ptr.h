#pragma once
#include "internal/safe-ptr/safe-ptr.h"

namespace vshalygin::cl {
    template<typename T>
    class safe_ptr
    {
        static_assert(std::is_same_v<T, std::remove_reference_t<T>>,
                      "safe_ptr cannot store references");

    public:
        safe_ptr() = default;
        safe_ptr(T *val)
            : m_controller(std::make_shared<internal::safe_ptr_controller<T>>(val))
        {}

        safe_ptr(const safe_ptr &) = default;
        safe_ptr &operator=(const safe_ptr &) = default;
        safe_ptr(safe_ptr &&) = default;
        safe_ptr &operator=(safe_ptr &&) = default;

        auto operator->() const
        {
            return m_controller->create_proxy();
        }

        void lock()
        {
            m_controller->lock();
        }

        void unlock() noexcept
        {
            m_controller->unlock();
        }

        operator bool() const noexcept
        {
            return static_cast<bool>(m_controller);
        }

        void reset(T *val)
        {
            m_controller = std::make_shared<internal::safe_ptr_controller<T>>(val);
        }

        void reset()
        {
            m_controller.reset();
        }

        void swap(safe_ptr &other) noexcept
        {
            m_controller.swap(other.m_controller);
        }

        long use_count() const noexcept
        {
            return m_controller.use_count();
        }

    private:
        std::shared_ptr<internal::safe_ptr_controller<T>> m_controller;
    };

    template<typename T, typename...Args>
    auto make_safe(Args&&...args)
    {
        return safe_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
