#pragma once
#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <memory>

namespace vsh::example {
    class irpc_client_transport;

    class rpc_client_channel final
        : public google::protobuf::RpcChannel
    {
        using MethodDescriptor = google::protobuf::MethodDescriptor;
        using RpcController = google::protobuf::RpcController;
        using Message = google::protobuf::Message;
        using Closure = google::protobuf::Closure;

    public:
        explicit rpc_client_channel(std::shared_ptr<irpc_client_transport> transport);
        ~rpc_client_channel();

        rpc_client_channel(rpc_client_channel &) = delete;
        rpc_client_channel &operator=(rpc_client_channel &) = delete;

        void CallMethod(const MethodDescriptor *method,
                        RpcController *controller,
                        const Message *request,
                        Message *response,
                        Closure *done) override;

    private:
        std::shared_ptr<irpc_client_transport> transport_;
    };
}
