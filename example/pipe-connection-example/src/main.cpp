#include <iostream>
#include "server/server.h"
#include "client/client.h"
#include <common-lib/thread-pool/thread-pool.h>
#include <common-lib/pipe/pipe-env.h>

#include <iostream>
#include <thread>
#include <chrono>

using namespace vshalygin;

int main()
{
    auto thread_pool = std::make_shared<cl::thread_pool>(6);
    auto listener_pipe_name = "listener_pipe_name";
    auto pipe_env = std::make_shared<cl::pipe_env>(thread_pool);
    
    example::server server(thread_pool, pipe_env, listener_pipe_name);
    example::client client(thread_pool, pipe_env, listener_pipe_name);

    for(int i = 0; i < 100; ++i) {
        proto::client_request req;
        req.set_data("data1");
        auto ans = client.ask_data(req);
        std::cout << ans->data() << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
