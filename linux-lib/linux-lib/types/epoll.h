#pragma once
#ifdef __linux__

#include <linux-lib/types/unique-fd.h>

#include <sys/epoll.h>

#include <cstdint>
#include <system_error>

namespace vshalygin::linux {
    class epoll final
    {
    public:
        epoll();

        epoll(const epoll &) = delete;
        epoll &operator=(const epoll &) = delete;

        epoll(epoll &&) noexcept = default;
        epoll &operator=(epoll &&) noexcept = default;

        void add(unique_fd::fd_type fd,
                 uint32_t events,
                 void *data);
        void add(unique_fd::fd_type fd,
                 uint32_t events,
                 void *data,
                 std::error_code &ec) noexcept;

        void modify(unique_fd::fd_type fd,
                    uint32_t events,
                    void *data);
        void modify(unique_fd::fd_type fd,
                    uint32_t events,
                    void *data,
                    std::error_code &ec) noexcept;

        void remove(unique_fd::fd_type fd);
        void remove(unique_fd::fd_type fd, std::error_code &ec) noexcept;

        int wait(epoll_event *events,
                 int capacity,
                 int timeout_ms = -1);
        int wait(epoll_event *events,
                 int capacity,
                 std::error_code &ec,
                 int timeout_ms = -1) noexcept;

        unique_fd::fd_type get() const noexcept;

    private:
        unique_fd m_fd;
    };
}

#endif
