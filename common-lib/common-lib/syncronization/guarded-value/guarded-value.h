#pragma once
#include <mutex>
#include <utility>

namespace vsh::cl {
    template<typename T>
    class guarded_value final
    {
    public:
        guarded_value(T &&value)
            : m_value(std::move(value))
        {}

        template<typename...Args>
        guarded_value(Args&&...args)
            : m_value(std::forward<Args>(args)...)
        {}

        guarded_value(guarded_value &) = delete;
        guarded_value &operator=(guarded_value &) = delete;

        std::pair<std::unique_lock<std::mutex>, T &> get()
        {
            return { std::unique_lock(m_mtx), m_value };
        }

        std::pair<std::unique_lock<std::mutex>, const T &> get() const
        {
            return { std::unique_lock(m_mtx), m_value };
        }

    private:
        mutable std::mutex m_mtx;
        T m_value;
    };
}
