#pragma once
#ifdef _WIN32
#include <Windows.h>

#include <type_traits>
#include <utility>

namespace vshalygin::win::internal {
    template<typename Traits>
    class unique_handle final
    {
    public:
        using handle_type = typename Traits::handle_type;

        static_assert(std::is_same_v<decltype(Traits::invalid()), handle_type>,
                      "Traits::invalid() must return handle_type");
        static_assert(std::is_same_v<decltype(Traits::close(std::declval<handle_type>())), void>,
                      "Traits::close(handle_type) must return void");
        static_assert(noexcept(Traits::invalid()),
                      "Traits::invalid() must be noexcept");
        static_assert(noexcept(Traits::close(std::declval<handle_type>())),
                      "Traits::close(handle_type) must be noexcept");

        unique_handle() noexcept
            : m_handle(Traits::invalid())
        {}

        explicit unique_handle(handle_type h) noexcept
            : m_handle(h)
        {}

        ~unique_handle() noexcept
        {
            reset();
        }

        unique_handle(const unique_handle &) = delete;
        unique_handle &operator=(const unique_handle &) = delete;

        unique_handle(unique_handle &&other) noexcept
            : m_handle(std::exchange(other.m_handle, Traits::invalid()))
        {}

        unique_handle &operator=(unique_handle &&other) noexcept
        {
            if(this != &other) {
                reset(std::exchange(other.m_handle, Traits::invalid()));
            }

            return *this;
        }

        void reset(handle_type h = Traits::invalid()) noexcept
        {
            if(m_handle == h) {
                return;
            }

            if(m_handle != Traits::invalid()) {
                Traits::close(m_handle);
            }

            m_handle = h;
        }

        [[nodiscard]] handle_type get() const noexcept
        {
            return m_handle;
        }

        [[nodiscard]] handle_type release() noexcept
        {
            return std::exchange(m_handle, Traits::invalid());
        }

        [[nodiscard]] handle_type *put() noexcept
        {
            reset();
            return &m_handle;
        }

        [[nodiscard]] handle_type *addressof() noexcept
        {
            return &m_handle;
        }

        explicit operator bool() const noexcept
        {
            return m_handle != Traits::invalid();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return m_handle == Traits::invalid();
        }

        void swap(unique_handle &other) noexcept
        {
            std::swap(m_handle, other.m_handle);
        }

        friend void swap(unique_handle &lhs, unique_handle &rhs) noexcept
        {
            lhs.swap(rhs);
        }

    private:
        handle_type m_handle;
    };
}

#endif
