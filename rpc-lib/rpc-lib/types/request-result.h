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
            case request_result::unknown_error:
                return "unknown_error";
            default:
                assert(!"unexpected request result");
                return "unknown_error";
        }
    }

    inline request_result request_result_from_string(const std::string &rc)
    {
        auto ans = request_result::unknown_error;
        if(rc == "ok") {
            ans = request_result::ok;
        } else if(rc == "timeout") {
            ans = request_result::timeout;
        } else if(rc == "send_unknown_error") {
            ans = request_result::send_unknown_error;
        } else if(rc == "send_timeout_error") {
            ans = request_result::send_timeout_error;
        } else if(rc == "send_canceled_error") {
            ans = request_result::send_canceled_error;
        } else if(rc == "canceled") {
            ans = request_result::canceled;
        } else if (rc == "request_not_processed") {
            ans = request_result::request_not_processed;
        } else if(rc == "response_parse_error") {
            ans = request_result::response_parse_error;
        } else if(rc == "unknown_error") {
            ans = request_result::unknown_error;
        } else {
            assert(!"unexpected request result string");
        }
        return ans;
    }
}
