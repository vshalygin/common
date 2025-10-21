#include "rpc-service.h"
#include "rpc-server/rpc-server-transport/irpc-server-transport.h"

#include <memory>

namespace vsh::example {
    rpc_service::rpc_service(std::unique_ptr<irpc_server_transport> transport)
        : transport_(std::move(transport))
    {}

    void rpc_service::GetUser(::google::protobuf::RpcController * /*controller*/,
                              const ::vsh::example::proto::GetUserRequest * /*request*/,
                             ::vsh::example::proto::GetUserResponse *response,
                             ::google::protobuf::Closure *done)
    {
        if(response) {
            response->Clear();
            response->set_user_id(1);
            response->set_name("Ivan Ivanov");
        }

        if(done) {
            done->Run();
        }
    }

    void rpc_service::run()
    {
        while(true) {
            std::string buff;
            transport_->recv(buff);

            auto req_descr = descriptor()->method(0)->input_type();
            auto res_descr = descriptor()->method(0)->output_type();

            auto req = google::protobuf::MessageFactory::generated_factory()->GetPrototype(req_descr)->New();
            std::unique_ptr<google::protobuf::Message> request(req);
            request->ParseFromString(buff);

            auto res = google::protobuf::MessageFactory::generated_factory()->GetPrototype(res_descr)->New();
            std::unique_ptr<google::protobuf::Message> response(res);

            CallMethod(descriptor()->method(0), nullptr, request.get(), response.get(), nullptr);

            transport_->send(response->SerializeAsString());
        }
    }
}
