#include "rpc-server.h"

#include <rpc-lib/server/server-transport/iserver-transport.h>
#include <rpc-lib/common/transfer-entry/transfer-entry.h>

namespace vsh::example {
    rpc_server::rpc_server(std::unique_ptr<rpc::iserver_transport> transport,
                           std::unique_ptr<proto::Service> service)
        : m_transport(std::move(transport))
        , m_service(std::move(service))
    {}

    rpc_server::~rpc_server() = default;

    void rpc_server::run()
    {
        while(true) {
            cl::buffer buff;
            m_transport->recv(buff);
            cl::cbuffer_view cbv(buff.data(), buff.size());
            auto method_idx = static_cast<int>(rpc::get_method_idx_req(cbv));

            auto req_descr = m_service->descriptor()->method(method_idx)->input_type();
            auto res_descr = m_service->descriptor()->method(method_idx)->output_type();
            const auto serialized_message = rpc::get_serialized_message(cbv);
            const auto entry_number = rpc::get_entry_number_req(cbv);

            auto req = google::protobuf::MessageFactory::generated_factory()->GetPrototype(req_descr)->New();
            std::unique_ptr<google::protobuf::Message> request(req);
            request->ParseFromArray(serialized_message.data(),
                                    static_cast<int>(serialized_message.size()));

            auto res = google::protobuf::MessageFactory::generated_factory()->GetPrototype(res_descr)->New();
            std::unique_ptr<google::protobuf::Message> response(res);

            m_service->CallMethod(m_service->descriptor()->method(method_idx),
                                  nullptr,
                                  request.get(),
                                  response.get(),
                                  nullptr);

            auto transfer_entry = rpc::create_transfer_entry_res(entry_number, response.get());
            m_transport->send(std::move(transfer_entry));
        }
    }
}
