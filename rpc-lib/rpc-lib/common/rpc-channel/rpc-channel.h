#pragma once
#include "rpc-lib/common/channel/ichannel.h"

#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

namespace vsh::rpc {
    class rpc_channel final
        : public google::protobuf::RpcChannel
    {
        using MethodDescriptor = google::protobuf::MethodDescriptor;
        using RpcController = google::protobuf::RpcController;
        using Message = google::protobuf::Message;
        using Closure = google::protobuf::Closure;

    public:
        explicit rpc_channel(std::unique_ptr<ichannel> channel);

        rpc_channel(rpc_channel &) = delete;
        rpc_channel &operator=(rpc_channel &) = delete;

        void CallMethod(const MethodDescriptor *method,
                        RpcController *controller,
                        const Message *request,
                        Message *response,
                        Closure *done) override;

    private:
        std::unique_ptr<ichannel> channel_;
    };
}
