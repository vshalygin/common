#include "client-base.h"
#include "rpc-lib/client/client-connection/iclient-connection.h"
#include "rpc-lib/common/channel/channel.h"
#include "rpc-lib/common/channel/transfer-entry-creator/transfer-entry-req-creator.h"
#include "rpc-lib/client/client-recv-handler/client-recv-handler.h"
#include "rpc-lib/common/listener/listener.h"

namespace vsh::rpc {
    namespace {
        std::unique_ptr<ilistener> create_listener(auto cb_map,
                                                   auto transport,
                                                   auto thread_pool)
        {
            auto recv_handler = std::make_shared<client_recv_handler>(std::move(cb_map));
            return std::make_unique<listener>(std::move(recv_handler),
                                              std::move(transport),
                                              std::move(thread_pool));
        }

        std::unique_ptr<google::protobuf::RpcChannel> create_channel(auto transport,
                                                                     auto thread_pool,
                                                                     auto cb_map,
                                                                     const std::string &client_id_)
        {
            auto entry_creator = std::make_unique<transfer_entry_req_creator>(client_id_);
            return std::make_unique<channel>(std::move(transport),
                                             std::move(thread_pool),
                                             std::move(cb_map),
                                             std::move(entry_creator));
        }
    }

    client_base::client_base(std::shared_ptr<cl::ithread_pool> thread_pool,
                             std::shared_ptr<iclient_connection> connection,
                             std::shared_ptr<itransport> transport,
                             std::unique_ptr<ilistener> server_listener)
        : m_client_id("5B576C99271E4516BD509B9B8C363327") //TODO generate
        , m_cb_map(std::make_shared<guarded_cb_map>())
        , m_thread_pool(std::move(thread_pool))
        , m_connection(std::move(connection))
        , m_channel(create_channel(transport, m_thread_pool, m_cb_map, m_client_id))
        , m_server_listener(std::move(server_listener))
    {}

    client_base::client_base(std::shared_ptr<cl::ithread_pool> thread_pool,
                             std::shared_ptr<iclient_connection> connection,
                             std::shared_ptr<itransport> transport)
        : client_base(thread_pool, connection, transport, nullptr)
    {
        m_server_listener = create_listener(m_cb_map, transport, thread_pool);
    }

    int client_base::connect()
    {
        m_server_listener->start(); //TODO think about it later
        return m_connection->connect();
    }

    int client_base::disconnect()
    {
        return m_connection->close();
    }

    ::google::protobuf::RpcChannel *client_base::get_channel() const
    {
        return m_channel.get();
    }
}
