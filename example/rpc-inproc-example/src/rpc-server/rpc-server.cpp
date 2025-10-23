#include "rpc-server.h"

#include <rpc-lib/server/server-transport/iserver-transport.h>

namespace vsh::example {
    rpc_server::rpc_server(std::unique_ptr<rpc::iserver_transport> transport,
                           std::unique_ptr<proto::Service> service)
        : transport_(std::move(transport))
        , service_(std::move(service))
    {}

    rpc_server::~rpc_server() = default;

    void rpc_server::run()
    {
        while(true) {
            std::string buff;
            transport_->recv(buff);

            auto req_descr = service_->descriptor()->method(0)->input_type();
            auto res_descr = service_->descriptor()->method(0)->output_type();

            auto req = google::protobuf::MessageFactory::generated_factory()->GetPrototype(req_descr)->New();
            std::unique_ptr<google::protobuf::Message> request(req);
            request->ParseFromString(buff);

            auto res = google::protobuf::MessageFactory::generated_factory()->GetPrototype(res_descr)->New();
            std::unique_ptr<google::protobuf::Message> response(res);

            service_->CallMethod(service_->descriptor()->method(0),
                                 nullptr,
                                 request.get(),
                                 response.get(),
                                 nullptr);

            transport_->send(response->SerializeAsString());
        }
    }
}
