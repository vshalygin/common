#pragma once
#include <string>
#include <cassert>

namespace vsh::rpc {
    enum class response_result : unsigned char
    {
        ok = 0,
        canceled,
        insufficient_rights,
        request_parse_error,
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
            case response_result::insufficient_rights:
                return "insufficient_rights";
            case response_result::request_parse_error:
                return "request_parse_error";
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
        } else if(rc == "insufficient_rights") {
            ans = response_result::insufficient_rights;
        } else if(rc == "request_parse_error") {
            ans = response_result::request_parse_error;
        } else if(rc == "unknown_error") {
            ans = response_result::unknown_error;
        } else {
            assert(!"unexpected response result string");
        }
        return ans;
    }
}
