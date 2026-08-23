#pragma once
#include "client-service.h"

#include <rpc-lib/client-endpoint.h>

#include <string>

namespace vshalygin::example {
    class client
    {
    public:
        client(std::shared_ptr<cl::thread_pool> thread_pool,
               std::shared_ptr<rpc::iauthenticator> authenticator,
               std::shared_ptr<rpc::iclient_pipe_env> pipe_env,
               uint64_t id)
            : m_endpoint(std::move(thread_pool),
                         std::move(authenticator),
                         std::move(pipe_env),
                         std::make_shared<client_service>(id))
            , m_id(id)
        {
            m_endpoint.connect_async(std::chrono::seconds(10)).get();
        }

        client(const client &) = delete;
        client &operator=(const client &) = delete;

        void send(const std::string &msg)
        {
            using stub_method = decltype(&proto::server_service_Stub::accept_message);

            proto::message message;
            message.set_data(msg);

            m_endpoint.make_request<proto::message, proto::null_message, stub_method>(
                &proto::server_service_Stub::accept_message, message)
                .get()
                .apply([this](rpc::request_result r, std::unique_ptr<proto::null_message>) {
                          if(is_success(r)) {
                              write_to_console("client " + std::to_string(m_id) + " successfully sent a message");
                          } else {
                              write_to_console("client " + std::to_string(m_id) + " failed to send a message: " +
                                               to_string(r));
                          }
                       });
        }

    private:
        rpc::client_endpoint<proto::server_service_Stub, proto::client_service> m_endpoint;

        const uint64_t m_id;
    };
}
