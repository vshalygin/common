#include "rpc-service.h"

#include <memory>

namespace vsh::example {

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

    void rpc_service::GetUser2(::google::protobuf::RpcController * /*controller*/,
                               const ::vsh::example::proto::GetUserRequest * /*request*/,
                               ::vsh::example::proto::GetUserResponse *response,
                               ::google::protobuf::Closure *done)
    {
        if(response) {
            response->Clear();
            response->set_user_id(1);
            response->set_name("Petr Petrov");
        }

        if(done) {
            done->Run();
        }
    }
}
