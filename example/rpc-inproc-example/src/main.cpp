#include <rpc-client/rpc-client-transport/rpc-client-transport.h>
#include <rpc-client/rpc-client-channel/rpc-client-channel.h>
#include <rpc-client/rpc-client.h>

#include <rpc-server/rpc-server-transport/rpc-server-transport.h>
#include <rpc-server/rpc-service/rpc-service.h>
#include <rpc-server/rpc-server.h>

#include <common-lib/thread-pool/thread-pool.h>

#include <iostream>
#include <thread>

using namespace vsh::example;

int main()
{
    auto thread_pool = std::make_shared<vsh::common_lib::thread_pool>(4);

    auto server_transport = std::make_unique<rpc_server_transport>();
    auto service = std::make_unique<rpc_service>();
    auto server = std::make_shared<rpc_server>(std::move(server_transport), std::move(service));
    auto thread = std::jthread([server]() { server->run(); });

    auto client_transport = std::make_shared<rpc_client_transport>();
    auto channel = std::make_unique<rpc_client_channel>(client_transport, thread_pool);
    auto client = std::make_unique<rpc_client>(std::move(channel), client_transport);

    client->connect();
    while(true) {
        auto ans = client->get_user({});
        std::cout << ans->name() << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
