#include "connection-watcher.h"

namespace vshalygin::rpc {
    connection_watcher::connection_watcher(std::chrono::milliseconds ping_period)
        : m_ping_period(ping_period)
    {}

    bool connection_watcher::check_and_drop_activity_flag() noexcept
    {
        auto r = m_activity_flag;
        m_activity_flag = false;
        return r;
    }

    void connection_watcher::set_activity_flag() noexcept
    {
        m_activity_flag = true;
        m_start_ping_waiting.reset();
    }

    void connection_watcher::set_ping_waiting() noexcept
    {
        if(!m_start_ping_waiting) {
            m_start_ping_waiting = std::chrono::steady_clock::now();
        }
    }

    bool connection_watcher::is_connection_not_responding() const noexcept
    {
        return m_start_ping_waiting &&
               std::chrono::steady_clock::now() > *m_start_ping_waiting + m_ping_period;
    }
}
