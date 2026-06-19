#pragma once
#include <string>
#include <cassert>

namespace vshalygin::rpc {
    enum class pipe_wait_res
    {
        connected,
        invalidated,
        timeout
    };

    inline bool is_success(pipe_wait_res r)
    {
        return r == pipe_wait_res::connected;
    }

    inline bool is_fail(pipe_wait_res r)
    {
        return !is_success(r);
    }

    inline std::string to_string(pipe_wait_res r)
    {
        switch(r) {
            case pipe_wait_res::connected:
                return "connected";
            case pipe_wait_res::invalidated:
                return "invalidated";
            case pipe_wait_res::timeout:
                return "timeout";
            default:
                assert(!"unknown pipe_wait_res");
                return "unknown";
        }
    }
}
