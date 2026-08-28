#include "channel.h"
#include <rpc-lib/closure-guard/closure-guard.h>
#include <rpc-lib/internal/connection/iconnection.h>
#include <rpc-lib/internal/transfer-message/transfer-message.h>
#include <rpc-lib/internal/controller/irequest-controller.h>

#pragma warning(push, 0)
#include <google/protobuf/message.h>
#pragma warning(pop)

#include <string>

namespace vshalygin::rpc::internal {
    channel::channel(std::unique_ptr<iconnection> &&connection)
        : m_connection(std::move(connection))
    {
        assert(m_connection);
    }

    channel::~channel() = default;

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

        closure_guard cg(done);
        auto request_controller = to_request_controller(controller);

        if(is_request_proto_too_big(request)) {
            request_controller->set_result(request_result::request_too_big);
            return;
        }

        const auto req_id = m_next_req_id.fetch_add(1);
        const auto method_idx = static_cast<uint32_t>(method->index());
        m_connection->request_async(create_transfer_msg_req(req_id, method_idx, request))
            .then([cg = std::move(cg), request_controller, response, req_id] (auto value) mutable {
                return value.lock().with(
                    [&](request_result rc, cl::buffer &&buffer) mutable {
                        auto guard = std::move(cg);

                        try {
                            if(is_success(rc)) {
                                if(!is_response_buffer_valid(buffer)) {
                                    request_controller->set_result(request_result::invalid_response);
                                    return;
                                }

                                if(get_transfer_msg_type(buffer) != transfer_msg_type::res) {
                                    throw std::runtime_error(
                                        "unexpected transfer_msg_type: " +
                                        std::to_string(static_cast<int>(get_transfer_msg_type(buffer))));
                                }

                                if(get_msg_number_res(buffer) != req_id) {
                                    throw std::runtime_error(
                                        "unexpected message number: " +
                                        std::to_string(get_msg_number_res(buffer)));
                                }

                                response->Clear();

                                if(auto res_code = get_msg_response_code_res(buffer); is_fail(res_code)) {
                                    request_controller->set_result(request_result::request_not_processed);
                                    return;
                                }

                                const auto serialized_response = get_serialized_proto_message(buffer);
                                const auto parse_result = response->ParseFromArray(
                                    static_cast<const void *>(serialized_response.data()),
                                    static_cast<int>(serialized_response.size()));

                                if(!parse_result) {
                                    request_controller->set_result(request_result::response_parse_error);
                                    return;
                                }

                                request_controller->set_result(request_result::ok);
                            } else {
                                request_controller->set_result(rc);
                            }
                        } catch(...) {
                            request_controller->set_result(request_result::unknown_error);
                        }
                    });
            });
    }

    void channel::start()
    {
        m_connection->start();
    }

    void channel::disconnect()
    {
        m_connection->deactivate();
    }

    bool channel::is_connected() const
    {
        return m_connection->is_active();
    }

    void channel::set_disconnect_callback(cl::thread_pool_task<void()> &&callback)
    {
        m_connection->set_stop_callback(std::move(callback));
    }
}
