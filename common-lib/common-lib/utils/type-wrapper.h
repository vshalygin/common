#pragma once
#include <common-lib/mpl/type-transform.h>
#include <common-lib/mpl/type-traits.h>
#include <type_traits>

namespace vshalygin::cl {
    template<typename T, typename Enable = void>
    class type_wrapper final
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
                             std::is_same_v<remove_type_qualifiers_t<U>,
                                            remove_type_qualifiers_t<T>>, int> = 0>
        type_wrapper &operator=(U &&val)
        {
            if(std::addressof(val) != std::addressof(m_val)) {
                m_val = std::forward<U>(val);
            }
            return *this;
        }

        operator const T &() const
        {
            return to_underlying();
        }

        operator T &()
        {
            return to_underlying();
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

        template<typename U,
            std::enable_if_t<!is_const_v<T> &&
                              std::is_same_v<remove_type_qualifiers_t<U>,
                                             remove_type_qualifiers_t<T>>, int> = 0>
        type_wrapper &operator=(U &&val)
        {
            if(std::addressof(val) != std::addressof(m_val)) {
                m_val = std::forward<U>(val);
            }
            return *this;
        }

        operator add_const_t<T>() const
        {
            return to_underlying();
        }

        operator T()
        {
            return to_underlying();
        }

        add_const_t<T> to_underlying() const
        {
            return std::forward<T>(m_val);
        }

        T to_underlying()
        {
            return std::forward<T>(m_val);
        }

    private:
        T m_val;
    };
}
