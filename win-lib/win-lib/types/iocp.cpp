#ifdef _WIN32
#include "iocp.h"
#include <cassert>

namespace vshalygin::win {
    iocp::iocp(DWORD number_of_concurrent_threads)
        : m_handle(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, number_of_concurrent_threads))
    {
        if(!m_handle) {
            throw std::system_error(::GetLastError(), std::system_category());
        }
    }

    void iocp::associate(HANDLE h, ULONG_PTR key)
    {
        std::error_code ec;
        associate(h, key, ec);
        if(ec) {
            throw std::system_error(ec);
        }
    }

    void iocp::associate(HANDLE h, ULONG_PTR key, std::error_code &ec) noexcept
    {
        assert(h != INVALID_HANDLE_VALUE);
        assert(h != nullptr);

        ec.clear();
        HANDLE r = ::CreateIoCompletionPort(h, m_handle.get(), key, 0);
        if(r == NULL) {
            ec.assign(::GetLastError(), std::system_category());
        }
    }

    completion_status iocp::get(DWORD milliseconds) noexcept
    {
        completion_status ans;
        BOOL r = GetQueuedCompletionStatus(m_handle.get(),
                                           &ans.bytes_transferred,
                                           &ans.key,
                                           &ans.overlapped,
                                           milliseconds);
        if(r == FALSE) {
            ans.success = false;
            ans.error = ::GetLastError();
        }

        return ans;
    }

    void iocp::post(DWORD bytes_transferred, ULONG_PTR key, OVERLAPPED *overlapped)
    {
        std::error_code ec;
        post(bytes_transferred, key, overlapped, ec);
        if(ec) {
            throw std::system_error(ec);
        }
    }

    void iocp::post(DWORD bytes_transferred,
                    ULONG_PTR key,
                    OVERLAPPED *overlapped,
                    std::error_code &ec) noexcept
    {
        ec.clear();
        BOOL r = PostQueuedCompletionStatus(m_handle.get(), bytes_transferred, key, overlapped);
        if(r == FALSE) {
            ec.assign(::GetLastError(), std::system_category());
        }
    }

    iocp_handle::handle_type iocp::native_handle() const noexcept
    {
        return m_handle.get();
    }

    void iocp::reset() noexcept
    {
        m_handle.reset();
    }
};

#endif
