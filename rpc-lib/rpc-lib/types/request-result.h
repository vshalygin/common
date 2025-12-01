#pragma once
namespace vsh::rpc {
    enum class request_result : unsigned char
    {
        ok = 0,
        timeout,
        send_error,
        canceled,
        unknown_error
    };

    inline bool is_success(request_result rc)
    {
        return rc == request_result::ok;
    }

    inline bool is_error(request_result rc)
    {
        return !is_success(rc);
    }
}
