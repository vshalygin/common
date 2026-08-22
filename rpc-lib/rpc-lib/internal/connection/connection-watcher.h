#pragma once
#include <chrono>
#include <optional>

namespace vshalygin::rpc {
    class connection_watcher
    {
    public:
        connection_watcher(std::chrono::milliseconds ping_period);

        connection_watcher(const connection_watcher &) = delete;
        connection_watcher &operator=(const connection_watcher &) = delete;

        bool check_and_drop_activity_flag() noexcept;
        void set_activity_flag() noexcept;

        void set_ping_waiting() noexcept;

        bool is_connection_not_responding() const noexcept;

    private:
        const std::chrono::milliseconds m_ping_period;

        bool m_activity_flag = true;
        std::optional<std::chrono::steady_clock::time_point> m_start_ping_waiting;
    };
}
