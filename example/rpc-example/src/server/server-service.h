#pragma once
#include "../utils/utils.h"

#pragma warning(push, 0)
#include <services.pb.h>
#pragma warning(pop)

#include <rpc-lib/closure-guard.h>
#include <rpc-lib/iresponse-controller.h>

namespace vshalygin::example {
    class server_service
        : public proto::server_service
    {
    public:
        server_service() = default;

        server_service(const server_service &) = delete;
        server_service &operator=(const server_service &) = delete;

        void accept_message(::google::protobuf::RpcController *controller,
                            const proto::message *request,
                            proto::message *response,
                            ::google::protobuf::Closure *done) override
        {
            rpc::closure_guard closure_guard(done);

            auto connection_id = rpc::to_response_controller(controller)->get_connection_id();

            write_to_console("server received: '" + request->data() + "' from connection " +
                             std::to_string(connection_id) + "\n");
            response->set_data("server processed message");
        }
    };
}
