#pragma once
#include "../utils/utils.h"

#pragma warning(push, 0)
#include <services.pb.h>
#pragma warning(pop)

#include <rpc-lib/closure-guard.h>

namespace vshalygin::example {
    class client_service
        : public proto::client_service
    {
    public:
        explicit client_service(uint64_t client_id)
            : m_client_id(client_id)
        {}

        client_service(const client_service &) = delete;
        client_service &operator=(const client_service &) = delete;

        void accept_message(::google::protobuf::RpcController * /*controller*/,
                            const proto::message *request,
                            proto::message *response,
                            ::google::protobuf::Closure *done) override
        {
            rpc::closure_guard closure_guard(done);

            write_to_console("client " + std::to_string(m_client_id) + " received: '" + request->data() + "'\n");
            response->set_data("client " + std::to_string(m_client_id) + " processed message");
        }

    private:
        uint64_t m_client_id;
    };
}
