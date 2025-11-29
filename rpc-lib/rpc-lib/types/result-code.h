#pragma once
namespace vsh::rpc {
    enum class result_code : unsigned char
    {
        ok = 0,
        timeout,
        unknown_error
    };

    inline bool is_success(result_code rc)
    {
        return rc == result_code::ok;
    }

    inline bool is_failed(result_code rc)
    {
        return !is_success(rc);
    }
}
