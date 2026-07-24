#pragma once
#ifdef _WIN32
#include <Windows.h>

namespace vshalygin::rpc::internal {
    enum class win_pipe_operation_kind
    {
        create,
        read,
        write
    };

    struct win_pipe_operation
    {
        win_pipe_operation(win_pipe_operation_kind k)
            : kind(k)
        {}

        //contract: must be first
        OVERLAPPED overlapped{};

        win_pipe_operation_kind kind;
    };
}
#endif
