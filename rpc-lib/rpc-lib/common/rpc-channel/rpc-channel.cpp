#include "rpc-channel.h"

namespace vsh::rpc {
    rpc_channel::rpc_channel(std::unique_ptr<ichannel> channel)
        : channel_(std::move(channel))
    {}


    void rpc_channel::CallMethod(const MethodDescriptor *method,
                                 RpcController *controller,
                                 const Message *request,
                                 Message *response,
                                 Closure *done)
    {
        channel_->CallMethod(method, controller, request, response, done);
    }
}
