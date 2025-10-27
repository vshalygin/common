#include <rpc-client/rpc-client-transport/rpc-client-transport.h>
#include <rpc-client/rpc-client.h>
#include <rpc-lib/client/client-recv-handler/client-recv-handler.h>
#include <rpc-lib/common/listener/listener.h>

#include <rpc-server/rpc-server-transport/rpc-server-transport.h>
#include <rpc-server/rpc-service/rpc-service.h>
#include <rpc-server/rpc-server.h>

#include <rpc-lib/common/channel/channel.h>
#include <rpc-lib/common/channel/transfer-entry-creator/transfer-entry-req-creator.h>
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

    auto client = std::make_unique<rpc_client>(thread_pool);

    client->connect();
    while(true) {
        auto ans = client->get_user2({});
        std::cout << ans->name() << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
