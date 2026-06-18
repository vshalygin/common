#include "server-endpoint.h"
#include "rpc-lib/channel/channel.h"

#include <vector>

namespace vshalygin::rpc {
    void server_endpoint::start_listen()
    {
        m_listener->start();
    }

    bool server_endpoint::is_listening() const
    {
        return !m_listener->is_stopped();
    }

    void server_endpoint::stop_listen()
    {
        return m_listener->stop();
    }

    void server_endpoint::set_listener_change_state_handler
        (listener_change_state_handler_t &&handler)
    {
        m_listener->set_change_state_handler(std::move(handler));
    }

    void server_endpoint::drop_connection(uint64_t id)
    {
        m_listener->drop_connection(id);
    }

    void server_endpoint::drop_all_connections()
    {
        m_listener->drop_all_connections();
    }

}
