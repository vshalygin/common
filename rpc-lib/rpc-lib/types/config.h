#pragma once
#include <chrono>

namespace vshalygin::rpc {
    struct config
    {
        std::chrono::milliseconds handshake_timeout = std::chrono::seconds(2);
        std::chrono::milliseconds send_timeout = std::chrono::seconds(2);
        std::chrono::milliseconds recv_timeout = std::chrono::seconds(10);
        std::chrono::milliseconds check_connection_period = std::chrono::seconds(1);
        std::chrono::milliseconds ping_timeout = std::chrono::seconds(10);
    };
}
