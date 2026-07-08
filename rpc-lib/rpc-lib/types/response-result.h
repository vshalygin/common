#pragma once
#include <string>
#include <cassert>

namespace vshalygin::rpc {
    enum class response_result : unsigned char
    {
        ok = 0,
        canceled,
        request_parse_error,
        response_too_big,
        not_implemented,
        unknown_error
    };

    inline bool is_success(response_result rc)
    {
        return rc == response_result::ok;
    }

    inline bool is_fail(response_result rc)
    {
        return !is_success(rc);
    }

    inline std::string to_string(response_result rc)
    {
        switch(rc) {
            case response_result::ok:
                return "ok";
            case response_result::canceled:
                return "canceled";
            case response_result::request_parse_error:
                return "request_parse_error";
            case response_result::response_too_big:
                return "response_too_big";
            case response_result::not_implemented:
                return "not_implemented";
            case response_result::unknown_error:
                return "unknown_error";
            default:
                assert(!"unexpected response result");
                return "unknown_error";
        }
    }

    inline response_result response_result_from_string(const std::string &rc)
    {
        auto ans = response_result::unknown_error;
        if(rc == "ok") {
            ans = response_result::ok;
        } else if(rc == "canceled") {
            ans = response_result::canceled;
        } else if(rc == "request_parse_error") {
            ans = response_result::request_parse_error;
        } else if(rc == "response_too_big") {
            ans = response_result::response_too_big;
        } else if(rc == "not_implemented") {
            ans = response_result::not_implemented;
        } else if(rc == "unknown_error") {
            ans = response_result::unknown_error;
        } else {
            assert(!"unexpected response result string");
        }
        return ans;
    }
}
