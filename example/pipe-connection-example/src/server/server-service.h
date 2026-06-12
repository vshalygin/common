#pragma once
#pragma warning(push, 0)
#include "proto/service.pb.h"
#pragma warning(pop)

namespace vshalygin::example {
    class server_service
        : public proto::server_service
    {
    public:
        void get_data(::google::protobuf::RpcController *controller,
                      const ::proto::client_request *request,
                      ::proto::client_response *response,
                      ::google::protobuf::Closure * done) override;
    };
}
