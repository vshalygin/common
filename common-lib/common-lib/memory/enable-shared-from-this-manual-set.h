#pragma once
#include <memory>
#include <atomic>
#include <stdexcept>

namespace vshalygin::cl {
    template<typename T>
    class enable_shared_from_this_manual_set
    {
    public:
        //must called right after creation and never called again, otherwise UB
        void set_self_shared_ptr(const std::shared_ptr<T> &ptr)
        {
            m_ptr = ptr;
        }

    protected:
        enable_shared_from_this_manual_set() noexcept = default;
        ~enable_shared_from_this_manual_set() noexcept = default;

        enable_shared_from_this_manual_set(const enable_shared_from_this_manual_set &) noexcept
        {}

        enable_shared_from_this_manual_set &operator=(const enable_shared_from_this_manual_set &) noexcept
        {
            return *this;
        }

        std::shared_ptr<T> shared_from_this()
        {
            return std::shared_ptr<T>(m_ptr);
        }

        std::weak_ptr<T> weak_from_this() noexcept
        {
            return m_ptr;
        }

        std::shared_ptr<const T> shared_from_this() const
        {
            return std::shared_ptr<const T>(m_ptr);
        }

        std::weak_ptr<const T> weak_from_this() const noexcept
        {
            return m_ptr;
        }

    private:
        std::weak_ptr<T> m_ptr;
    };
}
