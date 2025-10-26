#include <rpc-client/rpc-client-transport/rpc-client-transport.h>
#include <rpc-client/rpc-client.h>
#include <rpc-lib/client/server-event-processor/server-event-processor.h>
#include <rpc-lib/common/listener/listener.h>

#include <rpc-server/rpc-server-transport/rpc-server-transport.h>
#include <rpc-server/rpc-service/rpc-service.h>
#include <rpc-server/rpc-server.h>

#include <rpc-lib/client/client-channel/client-channel.h>
#include <common-lib/thread-pool/thread-pool.h>

#include <iostream>
#include <thread>

using namespace vsh::example;
using namespace vsh::rpc;
using namespace vsh;

using callback_type = std::function<void(const common_lib::buffer &)>;
using guarded_cb_map = common_lib::guarded_value<std::unordered_map<uint64_t, callback_type>>;

int main()
{
    auto map = std::make_shared<guarded_cb_map>();

    auto thread_pool = std::make_shared<vsh::common_lib::thread_pool>(4);

    auto server_transport = std::make_unique<rpc_server_transport>();
    auto service = std::make_unique<rpc_service>();
    auto server = std::make_shared<rpc_server>(std::move(server_transport), std::move(service));
    auto thread = std::jthread([server]() { server->run(); });

    auto client_transport = std::make_shared<rpc_client_transport>();
    auto serv_event_processor = std::make_shared<server_event_processor>(map);
    auto server_listener = std::make_unique<rpc::listener>(serv_event_processor, client_transport, thread_pool);
    auto channel = std::make_unique<client_channel>(client_transport, thread_pool, map, "2A158D39610C4DE695A21A3B657EC039");
    auto client = std::make_unique<rpc_client>(std::move(channel), std::move(server_listener), client_transport);

    client->connect();
    while(true) {
        auto ans = client->get_user2({});
        std::cout << ans->name() << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
