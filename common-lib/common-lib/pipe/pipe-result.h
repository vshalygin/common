#pragma once

namespace vsh::cl {
    enum class pipe_result
    {
        ok,
        canceled,
        disabled,
        timeout,
        unknown_error
    };

    inline bool is_success(pipe_result res)
    {
        return res == pipe_result::ok;
    }

    inline bool is_fail(pipe_result res)
    {
        return !is_success(res);
    }
}
