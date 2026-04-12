#pragma once
#include "request-result.h"
#include <exception>

namespace vshalygin::rpc {
    class request_exception
        : public std::exception
    {
    public:
        request_exception(request_result rc, const char *message)
            : exception(message)
            , m_request_result(rc)
        {}

        request_result code() const
        {
            return m_request_result;
        }

    private:
        const request_result m_request_result;
    };
}
