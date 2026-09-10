
#ifdef __linux__

#include "epoll.h"

#include <sys/epoll.h>

#include <cerrno>
#include <cstdint>
#include <system_error>
#include <chrono>
#include <algorithm>

namespace vshalygin::linux {
    epoll::epoll()
        : m_fd(::epoll_create1(EPOLL_CLOEXEC))
    {
        if(!m_fd) {
            throw std::system_error(errno, std::generic_category(), "epoll_create1");
        }
    }

    void epoll::add(unique_fd::fd_type fd, uint32_t events, void *data)
    {
        std::error_code ec;
        add(fd, events, data, ec);
        if(ec) {
            throw std::system_error(ec, "epoll_ctl(ADD)");
        }
    }

    void epoll::add(unique_fd::fd_type fd,
                    uint32_t events,
                    void *data,
                    std::error_code &ec) noexcept
    {
        ec.clear();

        epoll_event event{};
        event.events = events;
        event.data.ptr = data;

        if(::epoll_ctl(m_fd.get(), EPOLL_CTL_ADD, fd, &event) == -1) {
            ec.assign(errno, std::generic_category());
        }
    }

    void epoll::modify(unique_fd::fd_type fd, uint32_t events, void *data)
    {
        std::error_code ec;
        modify(fd, events, data, ec);
        if(ec) {
            throw std::system_error(ec, "epoll_ctl(MOD)");
        }
    }

    void epoll::modify(unique_fd::fd_type fd,
                       uint32_t events,
                       void *data,
                       std::error_code &ec) noexcept
    {
        ec.clear();

        epoll_event event{};
        event.events = events;
        event.data.ptr = data;

        if(::epoll_ctl(m_fd.get(), EPOLL_CTL_MOD, fd, &event) == -1) {
            ec.assign(errno, std::generic_category());
        }
    }

    void epoll::remove(unique_fd::fd_type fd)
    {
        std::error_code ec;
        remove(fd, ec);
        if(ec) {
            throw std::system_error(ec, "epoll_ctl(DEL)");
        }
    }

    void epoll::remove(unique_fd::fd_type fd, std::error_code &ec) noexcept
    {
        ec.clear();

        if(::epoll_ctl(m_fd.get(), EPOLL_CTL_DEL, fd, nullptr) == -1) {
            ec.assign(errno, std::generic_category());
        }
    }

    int epoll::wait(epoll_event *events, int capacity, int timeout_ms)
    {
        std::error_code ec;
        const auto result = wait(events, capacity, ec, timeout_ms);
        if(ec) {
            throw std::system_error(ec, "epoll_wait");
        }

        return result;
    }

    int epoll::wait(epoll_event *events,
                    int capacity,
                    std::error_code &ec,
                    int timeout_ms) noexcept
    {
        ec.clear();

        int result;

        if(timeout_ms < 0) {
            do {
                result = ::epoll_wait(m_fd.get(),
                                      events,
                                      capacity,
                                      -1);
            } while(result == -1 && errno == EINTR);
        } else {
            using namespace std::chrono;
            const auto deadline = steady_clock::now() + milliseconds(timeout_ms);
            do {
                const auto remaining_ms =
                   std::max<int>(0, ceil<milliseconds>(deadline-steady_clock::now()).count());
                result = ::epoll_wait(m_fd.get(),
                                      events,
                                      capacity,
                                      remaining_ms);
            } while(result == -1 && errno == EINTR);
        }

        if(result == -1) {
            ec.assign(errno, std::generic_category());
        }

        return result;
    }

    unique_fd::fd_type epoll::get() const noexcept
    {
        return m_fd.get();
    }
}

#endif
