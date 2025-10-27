#pragma once
#include "transfer-entry-creator/itransfer-entry-creator.h"
#include "rpc-lib/common/transport/itransport.h"

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
    class channel
        : public ::google::protobuf::RpcChannel
    {
        using callback_type = std::function<void(const common_lib::buffer &)>;
        using guarded_cb_map = common_lib::guarded_value<std::unordered_map<uint64_t, callback_type>>;

        using ithread_pool = common_lib::ithread_pool;
        using MethodDescriptor = google::protobuf::MethodDescriptor;
        using RpcController = google::protobuf::RpcController;
        using Message = google::protobuf::Message;
        using Closure = google::protobuf::Closure;

    public:
        channel(std::shared_ptr<itransport> transport,
                std::shared_ptr<ithread_pool> thread_pool,
                std::shared_ptr<guarded_cb_map> cb_map,
                std::unique_ptr<itransfer_entry_creator> entry_creator);

        channel(channel &) = delete;
        channel &operator=(channel &) = delete;

        void CallMethod(const MethodDescriptor *method,
                        RpcController *controller,
                        const Message *request,
                        Message *response,
                        Closure *done) override;

    private:
        std::shared_ptr<itransport> transport_;
        std::shared_ptr<ithread_pool> thread_pool_;
        std::shared_ptr<guarded_cb_map> cb_map_;
        std::unique_ptr<itransfer_entry_creator> entry_creator_;
    };
}
