#pragma once
#include "ichannel.h"
#include "rpc-lib/types/request-result.h"
#include "rpc-lib/connection/connection.h"
#include <common-lib/synchronization/guarded-value/guarded-value.h>
#include <common-lib/utils/buffer.h>
#include <atomic>

namespace vshalygin::rpc {
    class channel final
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

        void set_connection(std::shared_ptr<connection> connection) override;
        std::shared_ptr<connection> get_connection() const override;
        void drop_connection() override;

    private:
        static void handler_response_event_unsafe([[maybe_unused]] uint64_t req_id,
                                                  RpcController *controller,
                                                  Message *response,
                                                  request_result rc,
                                                  cl::buffer &&buffer);

    private:
        std::atomic_uint64_t m_next_req_id = 0;
        cl::guarded_value<std::shared_ptr<connection>> m_connection;
    };
}

