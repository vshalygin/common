#pragma once
namespace vsh::rpc {
    enum class response_result : unsigned char
    {
        ok = 0,
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
}
