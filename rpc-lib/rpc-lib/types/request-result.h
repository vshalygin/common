#pragma once
#include <string>
#include <cassert>

namespace vshalygin::rpc {
    enum class request_result : unsigned char
    {
        ok,
        unknown_error,
        timeout,
        canceled,
        failed,
        send_timeout,
        send_canceled,
        send_failed,
        request_too_big,
        request_not_processed,
        invalid_response,
        response_parse_error,
        no_connection
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
            case request_result::canceled:
                return "canceled";
            case request_result::failed:
                return "failed";
            case request_result::send_timeout:
                return "send_timeout";
            case request_result::send_canceled:
                return "send_canceled";
            case request_result::send_failed:
                return "send_failed";
            case request_result::request_too_big:
                return "request_too_big";
            case request_result::request_not_processed:
                return "request_not_processed";
            case request_result::invalid_response:
                return "invalid_response";
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
