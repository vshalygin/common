#pragma once
#ifdef _WIN32
#include "handle.h"
#include <system_error>

namespace vshalygin::win {
    struct completion_status
    {
        bool success = true;
        DWORD bytes_transferred{};
        ULONG_PTR key{};
        OVERLAPPED *overlapped{};
        DWORD error = ERROR_SUCCESS;
    };

    class iocp
    {
    public:
        explicit iocp(DWORD number_of_concurrent_threads = 0);

        iocp(const iocp &) = delete;
        iocp &operator=(const iocp &) = delete;

        iocp(iocp &&) = default;
        iocp &operator=(iocp &&) = default;

        void associate(HANDLE h, ULONG_PTR key);
        void associate(HANDLE h, ULONG_PTR key, std::error_code &ec) noexcept;

        completion_status get(DWORD milliseconds = INFINITE) noexcept;

        void post(DWORD bytes_transferred, ULONG_PTR key, OVERLAPPED *overlapped);
        void post(DWORD bytes_transferred,
                  ULONG_PTR key,
                  OVERLAPPED *overlapped,
                  std::error_code &ec) noexcept;

        iocp_handle::handle_type native_handle() const noexcept;

    private:
        iocp_handle m_handle;
    };
};

#endif
