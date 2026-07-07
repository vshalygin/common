#include "channel.h"
#include "rpc-lib/closure-guard/closure-guard.h"
#include "rpc-lib/transfer-message/transfer-message.h"
#include "rpc-lib/controller/irequest-controller.h"

namespace vshalygin::rpc {
    channel::channel(std::unique_ptr<iconnection> &&connection)
        : m_connection(std::move(connection))
    {}

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

        auto cg = std::make_unique<closure_guard>(done);

        auto request_controller = to_request_controller(controller);
        const auto req_id = m_next_req_id.fetch_add(1);
        const auto method_idx = static_cast<uint32_t>(method->index());
        m_connection->request_async(create_transfer_msg_req(req_id, method_idx, request))
            .then(
        [cg = std::move(cg), request_controller, response, req_id] (request_result rc,
                                                                    cl::buffer &&buffer)
        {
            try {
                if(is_success(rc)) {
                    assert(get_transfer_msg_type(buffer) == transfer_msg_type::res);
                    assert(get_msg_number_res(buffer) == req_id);
                    response->Clear();

                    if(auto res_code = get_msg_response_code_res(buffer); is_fail(res_code)) {
                        request_controller->set_result(request_result::request_not_processed);
                        return;
                    }

                    const auto serialized_response = get_serialized_proto_message(buffer);
                    const auto parse_result =
                        response->ParseFromArray(
                            static_cast<const void *>(serialized_response.data()),
                            static_cast<int>(serialized_response.size()));

                    if(!parse_result) {
                        request_controller->set_result(request_result::response_parse_error);
                    }
                }
                else {
                    request_controller->set_result(rc);
                }
             } catch (...) {
                 request_controller->set_result(request_result::unknown_error);
             }
        });
    }
}
