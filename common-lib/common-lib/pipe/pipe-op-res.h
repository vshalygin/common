#pragma once
namespace vshalygin::cl {
    enum class pipe_op_res
    {
        success,
        canceled,
        failed
    };

    inline bool is_success(pipe_op_res res)
    {
        return res == pipe_op_res::success;
    }

    inline bool is_fail(pipe_op_res res)
    {
        return !is_success(res);
    }
}
