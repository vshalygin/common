#pragma once
#include <string>
#include <cassert>

namespace vshalygin::rpc {
    enum class pipe_wait_res
    {
        success,
        canceled
    };

    inline bool is_success(pipe_wait_res r)
    {
        return r == pipe_wait_res::success;
    }

    inline bool is_fail(pipe_wait_res r)
    {
        return !is_success(r);
    }

    inline std::string to_string(pipe_wait_res r)
    {
        switch(r) {
            case pipe_wait_res::success:
                return "success";
            case pipe_wait_res::canceled:
                return "canceled";
            default:
                assert(!"unknown pipe_wait_res");
                return "unknown";
        }
    }
}
