#pragma once
#include "type-transform.h"
#include <type_traits>

namespace vshalygin::cl {
    template<typename T, typename Enable = void>
    class type_wrapper
    {
    public:
        explicit type_wrapper(const T &val)
            : m_val(val)
        {}

        explicit type_wrapper(T &&val)
            : m_val(std::move(val))
        {}

        type_wrapper(const type_wrapper &) = default;
        type_wrapper &operator=(const type_wrapper &) = default;

        type_wrapper(type_wrapper &&) = default;
        type_wrapper &operator=(type_wrapper &&) = default;

        template<typename U,
            std::enable_if_t<!std::is_const_v<T> &&
                             std::is_same_v<remove_type_qualifiers_t<U>, T>, int> = 0>
        type_wrapper &operator=(U &&val)
        {
            if(std::addressof(val) != std::addressof(m_val)) {
                m_val = std::forward<U>(val);
            }
            return *this;
        }

        operator const T &() const
        {
            return m_val;
        }

        operator T &()
        {
            return m_val;
        }

        const T &to_underlying() const
        {
            return m_val;
        }

        T &to_underlying()
        {
            return m_val;
        }

    private:
        T m_val;
    };

    template<typename T>
    class type_wrapper<T, std::enable_if_t<std::is_reference_v<T>>>
    {
    public:
        explicit type_wrapper(T val)
            : m_val(std::forward<T>(val))
        {}

        type_wrapper(const type_wrapper &) = default;
        type_wrapper &operator=(const type_wrapper &) = delete;

        type_wrapper &operator=(const remove_c_ref_t<T> &val)
        {
            if(std::addressof(val) != std::addressof(m_val)) {
                m_val = val;
            }
            return *this;
        }

        type_wrapper &operator=(remove_c_ref_t<T> &&val)
        {
            if(std::addressof(val) != std::addressof(m_val)) {
                m_val = std::move(val);
            }
            return *this;
        }

        operator const T() const
        {
            return to_underlying();
        }

        operator T()
        {
            return to_underlying();
        }

        const T to_underlying() const
        {
            return m_val;
        }

        T to_underlying()
        {
            if constexpr(std::is_rvalue_reference_v<T>) {
                return std::move(m_val);
            } else {
                return m_val;
            }
        }

    private:
        T m_val;
    };
}
