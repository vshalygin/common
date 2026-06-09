#include "client-endpoint.h"

namespace vshalygin::rpc {
    client_endpoint::client_endpoint(std::shared_ptr<iservice> service,
                                     std::unique_ptr<ichannel> channel,
                                     std::shared_ptr<iconnection> connection,
                                     std::unique_ptr<iconnector> connector)
        : m_channel(std::move(channel))
        , m_connection(std::move(connection))
        , m_connector(std::move(connector))
    {
        using response_handler_t = iconnection::response_handler_t;

        assert(service);
        assert(m_channel);
        assert(m_connection);
        assert(m_connector);

        m_connection->set_request_handler(
            [service = std::move(service)](cl::buffer &&buffer,
                                           response_handler_t &&res_handler) {
                service->process_request(std::move(buffer), std::move(res_handler));
            });
    }

    void client_endpoint::connect()
    {
        auto transport = m_connector->create_transport();
        m_connection->set_and_start_transport(std::move(transport));
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

    void client_endpoint::set_connection_change_state_handler
        (std::function<void(connection_state)> &&handler)
    {
        m_connection->set_change_state_handler(std::move(handler));
    }
}
