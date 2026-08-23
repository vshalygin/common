#pragma once
#include "server-service.h"

#include <rpc-lib/server-endpoint.h>

#include <string>

namespace vshalygin::example {
    class server
    {
    public:
        server(std::shared_ptr<cl::thread_pool> thread_pool,
               std::shared_ptr<rpc::iauthenticator> authenticator,
               std::shared_ptr<rpc::iserver_pipe_env> pipe_env)
            : m_endpoint(&on_connection_change,
                         {},
                         std::move(thread_pool),
                         std::move(authenticator),
                         pipe_env,
                         std::make_shared<server_service>())
        {}

        server(const server &) = delete;
        server &operator=(const server &) = delete;

        void send(uint64_t connection_id, const std::string &msg)
        {
            using stub_method = decltype(&proto::client_service_Stub::accept_message);

            proto::message message;
            message.set_data(msg);

            m_endpoint.make_request<proto::message, proto::null_message, stub_method>(
                connection_id , &proto::client_service_Stub::accept_message, message)
                .get()
                .apply([this, connection_id](rpc::request_result r, std::unique_ptr<proto::null_message>) {
                           if(is_success(r)) {
                               write_to_console("server successfully sent a message to client with connection id " +
                                                std::to_string(connection_id));
                           } else {
                               write_to_console("server failed to send a message to client with connection id " +
                                                std::to_string(connection_id) + ": " + to_string(r));
                           }
                       });
        }

        void send_all(const std::string &msg)
        {
            using stub_method = decltype(&proto::client_service_Stub::accept_message);

            proto::message message;
            message.set_data(msg);

            auto futures = m_endpoint.make_request_all<proto::message, proto::null_message, stub_method>(
                &proto::client_service_Stub::accept_message, message);

            for(auto &f : futures) {
                auto connection_id = f.first;
                f.second
                    .get()
                    .apply([this, connection_id](rpc::request_result r, std::unique_ptr<proto::null_message>) {
                           if(is_success(r)) {
                               write_to_console("server successfully sent a message to client with connection id " +
                                                std::to_string(connection_id));
                           } else {
                               write_to_console("server failed to send a message to client with connection id " +
                                                std::to_string(connection_id) + ": " + to_string(r));
                           }
                       });
            }
        }

        size_t get_connections_count() const
        {
            return m_endpoint.get_active_connections_count();
        }

    private:
        static void on_connection_change(uint64_t id, rpc::connection_state s)
        {
            if(s == rpc::connection_state::connected) {
                write_to_console("new client connected to server with connection id: " + std::to_string(id));
            } else if (s == rpc::connection_state::disconnected) {
                write_to_console("client with connection id: " + std::to_string(id) + " disconnected from server");
            }
        }

    private:
        rpc::server_endpoint<proto::client_service_Stub, proto::server_service> m_endpoint;
    };
}
