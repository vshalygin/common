#pragma once
#include "rpc-lib/types/request-result.h"
#include "rpc-lib/connection/connection.h"
#include <common-lib/utils/buffer.h>

#pragma warning(push, 0)
#include <google/protobuf/service.h>
#pragma warning(pop)

#include <atomic>

namespace vshalygin::rpc {
    class channel final
        : public google::protobuf::RpcChannel
    {
        using MethodDescriptor = google::protobuf::MethodDescriptor;
        using RpcController = google::protobuf::RpcController;
        using Message = google::protobuf::Message;
        using Closure = google::protobuf::Closure;

    public:
        explicit channel(connection &&connection);

        channel(channel &) = delete;
        channel &operator=(channel &) = delete;

        void CallMethod(const MethodDescriptor *method,
                        RpcController *controller,
                        const Message *request,
                        Message *response,
                        Closure *done) override;

    private:
        static void handler_response_event_unsafe([[maybe_unused]] uint64_t req_id,
                                                  RpcController *controller,
                                                  Message *response,
                                                  request_result rc,
                                                  cl::buffer &&buffer);

    private:
        std::atomic_uint64_t m_next_req_id = 0;
        connection m_connection;
    };
}

