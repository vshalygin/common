#pragma once
#include <common-lib/thread/thread-pool/thread-pool-task.h>

#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <atomic>
#include <memory>

namespace vshalygin::rpc::internal {
    class iconnection;

    class channel final
        : public google::protobuf::RpcChannel
    {
        using MethodDescriptor = google::protobuf::MethodDescriptor;
        using RpcController = google::protobuf::RpcController;
        using Message = google::protobuf::Message;
        using Closure = google::protobuf::Closure;

    public:
        explicit channel(std::unique_ptr<iconnection> &&connection);

        channel(channel &) = delete;
        channel &operator=(channel &) = delete;

        ~channel();

        void CallMethod(const MethodDescriptor *method,
                        RpcController *controller,
                        const Message *request,
                        Message *response,
                        Closure *done) override;

        void start();

        void disconnect();
        bool is_connected() const;
        void set_disconnect_callback(cl::thread_pool_task<void()> &&callback);

    private:
        std::atomic_uint64_t m_next_req_id = 0;
        std::unique_ptr<iconnection> m_connection;
    };
}
