#pragma once
#include "ichannel.h"
#include "rpc-lib/types/request-result.h"
#include <common-lib/syncronization/guarded-value/guarded-value.h>
#include <common-lib/utils/buffer/buffer.h>
#include <atomic>

namespace vsh::rpc {
    class channel
        : public ichannel
    {
        using MethodDescriptor = google::protobuf::MethodDescriptor;
        using RpcController = google::protobuf::RpcController;
        using Message = google::protobuf::Message;
        using Closure = google::protobuf::Closure;

    public:
        channel() = default;

        channel(channel &) = delete;
        channel &operator=(channel &) = delete;

        void CallMethod(const MethodDescriptor *method,
                        RpcController *controller,
                        const Message *request,
                        Message *response,
                        Closure *done) override;

        void set_connection(std::shared_ptr<iconnection> connection) override;
        void drop_connection() override;

    private:
        static void handler_response_event_unsafe([[maybe_unused]] uint64_t req_id,
                                                  RpcController *controller,
                                                  Message *response,
                                                  request_result rc,
                                                  cl::buffer &&buffer);

    private:
        std::atomic_uint64_t m_next_req_id = 0;
        cl::guarded_value<std::shared_ptr<iconnection>> m_connection;
    };
}

