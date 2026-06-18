#include "client-endpoint.h"
#include "rpc-lib/transport/itransport.h"
#include "rpc-lib/connection/connection.h"

namespace vshalygin::rpc {
    client_endpoint::client_endpoint(std::shared_ptr<iservice> service,
                                     std::unique_ptr<ichannel> channel,
                                     std::unique_ptr<iconnector> connector,
                                     std::shared_ptr<cl::thread_pool> thread_pool,
                                     connection_state_change_handler_t &&state_change_handler)
        : m_service(std::move(service))
        , m_channel(std::move(channel))
        , m_connector(std::move(connector))
        , m_thread_pool(std::move(thread_pool))
        , m_connection_change_handler(state_change_handler) //TODO сейчас это будет работать 1 раз
    {}

    void client_endpoint::connect()
    {
        auto connection0 = std::make_shared<connection>(m_connector,
                                                        m_thread_pool,
                                                        m_service,
                                                        std::move(m_connection_change_handler));
        connection0->activate();
        m_channel->set_connection(connection0);
    }

    void client_endpoint::disconnect()
    {
        m_channel->get_connection()->deactivate(); //TODO продумать
    }

    bool client_endpoint::is_connected() const
    {
        return m_channel->get_connection()->is_active();
    }
}
