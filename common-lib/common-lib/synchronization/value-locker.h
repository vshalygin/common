#pragma once
#include <memory>
#include <mutex>

namespace vshalygin::cl {
    template<typename T>
    class locked_value
    {
    public:
        locked_value(std::unique_lock<std::mutex> lock,
                     T &value)
            : m_lock(std::move(lock))
            , m_value(value)
        {}

        locked_value(const locked_value &) = delete;
        locked_value &operator=(const locked_value &) = delete;

        T *operator->()
        {
            return &m_value;
        }

        const T *operator->() const
        {
            return &m_value;
        }

        T &operator*()
        {
            return m_value;
        }

        const T &operator*() const
        {
            return m_value;
        }

    private:
        std::unique_lock<std::mutex> m_lock;
        T &m_value;
    };

    template<typename T>
    class value_locker
    {
        static_assert(!std::is_reference_v<T>,
                      "reference types are not supported");

    public:
        template<typename...Args>
        value_locker(Args&&...args)
            : m_val(std::forward<Args>(args)...)
        {}
        
        value_locker(const value_locker &) = delete;
        value_locker &operator=(const value_locker &) = delete;

        auto lock()
        {
            std::unique_lock l(m_mtx);
            return locked_value<T>(std::move(l), m_val);
        }

        auto lock() const
        {
            std::unique_lock l(m_mtx);
            return locked_value<const T>(std::move(l), m_val);
        }

    private:
        mutable std::mutex m_mtx;
        T m_val;
    };
}
