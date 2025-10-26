#pragma once
#include "rpc-lib/client/client-transport/iclient-transport.h"

#include <common-lib/utils/guarded-value/guarded-value.h>
#include <common-lib/thread-pool/ithread-pool.h>
#include <common-lib/utils/buffer/buffer.h>

#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <memory>
#include <unordered_map>
#include <atomic>

namespace vsh::rpc {
    class client_channel
        : public ::google::protobuf::RpcChannel
    {
        using callback_type = std::function<void(const common_lib::buffer &)>;
        using guarded_cb_map = common_lib::guarded_value<std::unordered_map<unsigned, callback_type>>;

        using ithread_pool = common_lib::ithread_pool;
        using MethodDescriptor = google::protobuf::MethodDescriptor;
        using RpcController = google::protobuf::RpcController;
        using Message = google::protobuf::Message;
        using Closure = google::protobuf::Closure;

    public:
        client_channel(std::shared_ptr<iclient_transport> transport,
                       std::shared_ptr<ithread_pool> thread_pool,
                       std::shared_ptr<guarded_cb_map> cb_map,
                       const std::string &client_id);

        client_channel(client_channel &) = delete;
        client_channel &operator=(client_channel &) = delete;

        void CallMethod(const MethodDescriptor *method,
                        RpcController *controller,
                        const Message *request,
                        Message *response,
                        Closure *done) override;

    private:
        std::atomic_uint64_t counter_ = 0;
        const std::string client_id_;

        std::shared_ptr<iclient_transport> transport_;
        std::shared_ptr<ithread_pool> thread_pool_;
        std::shared_ptr<guarded_cb_map> cb_map_;

    };
}
