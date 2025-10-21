#include "rpc-client-channel.h"
#include "rpc-client/rpc-client-transport/irpc-client-transport.h"

#pragma warning(push, 0)
#include <google/protobuf/message.h>
#pragma warning(pop)

namespace vsh::example {
    rpc_client_channel::rpc_client_channel(std::shared_ptr<irpc_client_transport> transport)
        : transport_(std::move(transport))
    {}

    rpc_client_channel::~rpc_client_channel() = default;

    void rpc_client_channel::CallMethod(const MethodDescriptor * /*method*/,
                                        RpcController * /*controller*/,
                                        const Message *request,
                                        Message * response,
                                        Closure *done)
    {
        transport_->send(request->SerializeAsString());

        std::string ans;
        transport_->recv(ans);

        response->ParseFromString(ans);

        if(done) {
            done->Run();
        }
    }
}