#pragma once
#ifdef __linux__

#include <unistd.h>

#include <utility>

namespace vshalygin::linux {
    class unique_fd final
    {
    public:
        using fd_type = int;

        unique_fd() noexcept = default;

        explicit unique_fd(fd_type fd) noexcept
            : m_fd(fd)
        {}

        ~unique_fd() noexcept
        {
            reset();
        }

        unique_fd(const unique_fd &) = delete;
        unique_fd &operator=(const unique_fd &) = delete;

        unique_fd(unique_fd &&other) noexcept
            : m_fd(std::exchange(other.m_fd, invalid()))
        {}

        unique_fd &operator=(unique_fd &&other) noexcept
        {
            if(this != &other) {
                reset(std::exchange(other.m_fd, invalid()));
            }

            return *this;
        }

        void reset(fd_type fd = invalid()) noexcept
        {
            if(m_fd == fd) {
                return;
            }

            if(m_fd != invalid()) {
                ::close(m_fd);
            }

            m_fd = fd;
        }

        [[nodiscard]] fd_type get() const noexcept
        {
            return m_fd;
        }

        [[nodiscard]] fd_type release() noexcept
        {
            return std::exchange(m_fd, invalid());
        }

        [[nodiscard]] fd_type *put() noexcept
        {
            reset();
            return &m_fd;
        }

        [[nodiscard]] fd_type *addressof() noexcept
        {
            return &m_fd;
        }

        explicit operator bool() const noexcept
        {
            return m_fd != invalid();
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return m_fd == invalid();
        }

        void swap(unique_fd &other) noexcept
        {
            std::swap(m_fd, other.m_fd);
        }

        friend void swap(unique_fd &lhs, unique_fd &rhs) noexcept
        {
            lhs.swap(rhs);
        }

    private:
        static constexpr fd_type invalid() noexcept
        {
            return -1;
        }

        fd_type m_fd = invalid();
    };
}

#endif
