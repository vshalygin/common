#pragma once
namespace vsh::rpc {
    enum class response_result : unsigned char
    {
        ok = 0,
        insufficient_rights,
        unknown_error
    };

    inline bool is_success(response_result rc)
    {
        return rc == response_result::ok;
    }

    inline bool is_error(response_result rc)
    {
        return !is_success(rc);
    }
}
