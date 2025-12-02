#include "channel.h"
#include "closure-guard/closure-guard.h"
#include "rpc-lib/transfer-message/transfer-message.h"
#include "rpc-lib/connection/iconnection.h"

namespace vsh::rpc {
    void channel::CallMethod(const MethodDescriptor *method,
                             RpcController *controller,
                             const Message *request,
                             Message *response,
                             Closure *done)
    {
        assert(method);
        assert(controller);
        assert(request);
        assert(response);
        assert(done);

        const auto req_id = m_next_req_id.fetch_add(1);
        const auto method_idx = static_cast<uint32_t>(method->index());
        auto req_transfer_message = create_transfer_msg_req(req_id, method_idx, request);
        closure_guard cg(done);
        auto handler = [cg = std::move(cg), controller, response, req_id]
                       (request_result rc, cl::buffer &&buffer) {
            if(is_success(rc)) {
                assert(get_transfer_msg_type(buffer) == transfer_msg_type::res);
                assert(get_msg_number_res(buffer) == req_id);
                response->Clear();

                if(auto res_code = get_msg_response_code_res(buffer); is_fail(res_code)) {
                    controller->SetFailed(to_string(request_result::request_not_processed));
                    return;
                }

                const auto serialized_response = get_serialized_proto_message(buffer);
                const auto parse_result = response->ParseFromArray
                    (static_cast<const void *>(serialized_response.data()),
                     static_cast<int>(serialized_response.size()));

                if(!parse_result) {
                    controller->SetFailed(to_string(request_result::response_parse_error));
                }
            } else {
                controller->SetFailed(to_string(rc));
            }
        };

        auto [guard, connection] = m_connection.get();
        if(connection) {
            connection->request_async(std::move(req_transfer_message), std::move(handler));
        } else {
            throw std::runtime_error("connection is not set");
        }
    }

    void channel::set_connection(std::shared_ptr<iconnection> connection)
    {
        auto [guard, connect] = m_connection.get();
        connect = std::move(connection);
    }

    void channel::drop_connection()
    {
        auto [guard, connect] = m_connection.get();
        connect.reset();
    }
}
