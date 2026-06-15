#include "client-endpoint.h"
#include "rpc-lib/transport/itransport.h"
#include "rpc-lib/connection/connection.h"

namespace vshalygin::rpc {
    client_endpoint::client_endpoint(std::shared_ptr<iservice> service,
                                     std::unique_ptr<ichannel> channel,
                                     std::unique_ptr<iconnector> connector,
                                     std::shared_ptr<cl::thread_pool> thread_pool,
                                     connection_state_change_handler_t &&state_change_handler)
        : m_channel(std::move(channel))
        , m_connector(std::move(connector))
    {
        using response_handler_t = connection::response_handler_t;

        assert(m_channel);
        assert(m_connector);
        assert(thread_pool);

        m_connection = std::make_unique<connection>(thread_pool,
                                                    std::move(service),
                                                    std::move(state_change_handler));
    }

    void client_endpoint::connect()
    {
        auto transport = m_connector->create_transport();
        m_connection->start_and_set_transport(std::move(transport));
        m_channel->set_connection(m_connection);
    }

    void client_endpoint::disconnect()
    {
        m_connection->stop_transport();
    }

    bool client_endpoint::is_connected() const
    {
        return m_connection->is_active();
    }
}
