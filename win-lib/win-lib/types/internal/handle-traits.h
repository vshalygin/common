#pragma once
#ifdef _WIN32
#include <Windows.h>

namespace vshalygin::win::internal {
    struct event_handle_traits
    {
        using handle_type = HANDLE;

        static constexpr HANDLE invalid() noexcept
        {
            return NULL;
        }

        static void close(HANDLE h) noexcept
        {
            ::CloseHandle(h);
        }
    };

    struct iocp_handle_traits
    {
        using handle_type = HANDLE;

        static constexpr HANDLE invalid() noexcept
        {
            return NULL;
        }

        static void close(HANDLE h) noexcept
        {
            ::CloseHandle(h);
        }
    };

    struct pipe_handle_traits
    {
        using handle_type = HANDLE;

        static constexpr HANDLE invalid() noexcept
        {
            return INVALID_HANDLE_VALUE;
        }

        static void close(HANDLE h) noexcept
        {
            ::CloseHandle(h);
        }
    };
}
#endif
