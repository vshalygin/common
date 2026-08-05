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

    enum class win_pipe_operation_res
    {
        unknown,
        success,
        canceled,
        timeout,
        failed
    };

    struct win_pipe_overlapped
    {
        win_pipe_overlapped(win_pipe_operation_kind k)
            : kind(k)
        {}

        //contract: overlapped must be first
        OVERLAPPED overlapped{};

        win_pipe_operation_kind kind;
    };
}
#endif
