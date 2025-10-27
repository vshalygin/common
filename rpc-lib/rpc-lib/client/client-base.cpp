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

    client_base::client_base(std::shared_ptr<common_lib::ithread_pool> thread_pool,
                             std::shared_ptr<iclient_connection> connection,
                             std::shared_ptr<itransport> transport,
                             std::unique_ptr<ilistener> server_listener)
        : client_id_("5B576C99271E4516BD509B9B8C363327") //TODO generate
        , cb_map_(std::make_shared<guarded_cb_map>())
        , thread_pool_(std::move(thread_pool))
        , connection_(std::move(connection))
        , channel_(create_channel(transport, thread_pool_, cb_map_, client_id_))
        , server_listener_(std::move(server_listener))
    {}

    client_base::client_base(std::shared_ptr<common_lib::ithread_pool> thread_pool,
                             std::shared_ptr<iclient_connection> connection,
                             std::shared_ptr<itransport> transport)
        : client_base(thread_pool, connection, transport, nullptr)
    {
        server_listener_ = create_listener(cb_map_, transport, thread_pool);
    }

    int client_base::connect()
    {
        server_listener_->start(); //TODO think about it later
        return connection_->connect();
    }

    int client_base::disconnect()
    {
        return connection_->close();
    }

    ::google::protobuf::RpcChannel *client_base::get_channel() const
    {
        return channel_.get();
    }
}
