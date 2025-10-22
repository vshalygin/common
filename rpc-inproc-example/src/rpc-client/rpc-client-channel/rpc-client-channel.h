#pragma once
#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <memory>
#include <thread>

namespace vsh::common_lib {
    class ithread_pool;
}

namespace vsh::example {
    class irpc_client_transport;

    class rpc_client_channel final
        : public google::protobuf::RpcChannel
    {
        using ithread_pool = common_lib::ithread_pool;
        using MethodDescriptor = google::protobuf::MethodDescriptor;
        using RpcController = google::protobuf::RpcController;
        using Message = google::protobuf::Message;
        using Closure = google::protobuf::Closure;

    public:
        explicit rpc_client_channel(std::shared_ptr<irpc_client_transport> transport,
                                    std::shared_ptr<ithread_pool> thread_pool);
        ~rpc_client_channel();

        rpc_client_channel(rpc_client_channel &) = delete;
        rpc_client_channel &operator=(rpc_client_channel &) = delete;

        void CallMethod(const MethodDescriptor *method,
                        RpcController *controller,
                        const Message *request,
                        Message *response,
                        Closure *done) override;

    private:
        void listen_server();

    private:
        std::shared_ptr<irpc_client_transport> transport_;
        std::shared_ptr<ithread_pool> thread_pool_;

        std::jthread listen_thread_;

        std::function<void(const std::string &)> callback_;
    };
}
