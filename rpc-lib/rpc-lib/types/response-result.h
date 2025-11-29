#pragma once
namespace vsh::rpc {
    enum class response_result : unsigned char
    {
        ok = 0,
        rejected,
        unknown_error
    };

}
