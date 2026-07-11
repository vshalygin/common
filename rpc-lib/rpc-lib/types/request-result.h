#pragma once
#include <string>
#include <cassert>

namespace vshalygin::rpc {
    enum class request_result : unsigned char
    {
        ok = 0,
        timeout,
        canceled,
        send_timeout_error,
        send_canceled_error,
        send_unknown_error,
        request_not_processed,
        response_parse_error,
        no_connection,
        unknown_error
    };

    inline bool is_success(request_result rc)
    {
        return rc == request_result::ok;
    }

    inline bool is_fail(request_result rc)
    {
        return !is_success(rc);
    }

    inline std::string to_string(request_result rc)
    {
        switch(rc) {
            case request_result::ok:
                return "ok";
            case request_result::timeout:
                return "timeout";
            case request_result::send_timeout_error:
                return "send_timeout_error";
            case request_result::send_unknown_error:
                return "send_unknown_error";
            case request_result::send_canceled_error:
                return "send_canceled_error";
            case request_result::canceled:
                return "canceled";
            case request_result::request_not_processed:
                return "request_not_processed";
            case request_result::response_parse_error:
                return "response_parse_error";
            case request_result::no_connection:
                return "no_connection";
            case request_result::unknown_error:
                return "unknown_error";
            default:
                assert(!"unexpected request result");
                return "unknown_error";
        }
    }
}
