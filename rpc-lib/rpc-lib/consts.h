#pragma once
#include <cstdint>

namespace vshalygin::rpc {
    constexpr uint32_t MaxTransferMessageSize = 1024 * 1024 + 100; //1 MB + header and trailer

    constexpr uint32_t MaxRequestProtoSize = 1024 * 1024; //1 MB
    constexpr uint32_t MaxResponseProtoSize = 1024 * 1024; //1 MB
}
