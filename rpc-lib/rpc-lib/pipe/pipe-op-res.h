#pragma once
#include <string>

namespace vshalygin::rpc {
    enum class pipe_op_res
    {
        success,
        canceled,
        failed,
        timeout
    };

    inline bool is_success(pipe_op_res res)
    {
        return res == pipe_op_res::success;
    }

    inline bool is_fail(pipe_op_res res)
    {
        return !is_success(res);
    }

    inline std::string to_string(pipe_op_res r)
    {
        switch(r) {
            case pipe_op_res::success:
                return "success";
            case pipe_op_res::failed:
                return "failed";
            case pipe_op_res::canceled:
                return "canceled";
            case pipe_op_res::timeout:
                return "timeout";
            default:
                return "unknown";
        }
    }
}
