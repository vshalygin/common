#pragma once
#include "../server/server.h"
#include "../client/client.h"

#include <rpc-lib/authenticator/iauthenticator.h>
#include <rpc-lib/pipe/iclient-pipe-env.h>
#include <rpc-lib/pipe/iserver-pipe-env.h>

#include <common-lib/thread/thread-pool/thread-pool.h>

#include <memory>
#include <unordered_map>

namespace vshalygin::example {
    class app
    {
    public:
        app();

        app(const app &) = delete;
        app &operator=(const app &) = delete;

        ~app();

        int run() noexcept;

    private:
        void print_info();

    private:
        std::shared_ptr<cl::thread_pool> m_thread_pool;

        std::shared_ptr<rpc::iauthenticator> m_authenticator;
        std::shared_ptr<rpc::iclient_pipe_env> m_client_pipe_env;
        std::shared_ptr<rpc::iserver_pipe_env> m_server_pipe_env;

        std::unique_ptr<server> m_server;

        uint64_t m_next_client_id = 0;
        std::unordered_map<uint64_t, client> m_clients;
    };
}
