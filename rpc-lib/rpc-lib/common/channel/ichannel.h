#pragma once
#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

namespace vsh::rpc {
    class ichannel
    {
    public:
        virtual ~ichannel() = default;

        virtual void CallMethod(const ::google::protobuf::MethodDescriptor *method,
                                ::google::protobuf::RpcController *controller,
                                const ::google::protobuf::Message *request,
                                ::google::protobuf::Message *response,
                                ::google::protobuf::Closure *done) = 0;
    };
}
