#pragma once
#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <memory>
#include <thread>

namespace vsh::common_lib {
    class ithread_pool;
}

namespace vsh::rpc {
    class iclient_transport;

    class client_channel final
        : public google::protobuf::RpcChannel
    {
        using ithread_pool = common_lib::ithread_pool;
        using MethodDescriptor = google::protobuf::MethodDescriptor;
        using RpcController = google::protobuf::RpcController;
        using Message = google::protobuf::Message;
        using Closure = google::protobuf::Closure;

    public:
        explicit client_channel(std::shared_ptr<iclient_transport> transport,
                                std::shared_ptr<ithread_pool> thread_pool);
        ~client_channel();

        client_channel(client_channel &) = delete;
        client_channel &operator=(client_channel &) = delete;

        void CallMethod(const MethodDescriptor *method,
                        RpcController *controller,
                        const Message *request,
                        Message *response,
                        Closure *done) override;

    private:
        void listen_server();

    private:
        std::shared_ptr<iclient_transport> transport_;
        std::shared_ptr<ithread_pool> thread_pool_;

        std::jthread listen_thread_;

        std::function<void(const std::string &)> callback_;
    };
}
