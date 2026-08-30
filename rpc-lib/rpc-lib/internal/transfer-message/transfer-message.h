#pragma once
#include "rpc-lib/types/request-result.h"
#include "rpc-lib/types/response-result.h"

#include <common-lib/utils/cbuffer-view.h>
#include <common-lib/utils/buffer.h>

#include <string>

// A transfer message is one logically complete request, response, ping, or
// pong exchanged by two RPC connections. Its byte-level layout is centralized
// in transfer-message.cpp and described in the project's RPC architecture
// document.

namespace google::protobuf {
    class Message;
}

namespace vshalygin::rpc::internal {
    enum class transfer_msg_type : unsigned char
    {
        req = 0,
        res = 1,
        ping = 2,
        pong = 3
    };

    bool is_request_proto_too_big(const google::protobuf::Message *proto_message);
    bool is_response_proto_too_big(const google::protobuf::Message *proto_message);

    bool is_request_buffer_valid(cl::cbuffer_view buff) noexcept;
    bool is_response_buffer_valid(cl::cbuffer_view buff) noexcept;

    transfer_msg_type get_transfer_msg_type(cl::cbuffer_view message);
    cl::cbuffer_view get_serialized_proto_message(cl::cbuffer_view message);

    uint64_t get_msg_number_req(cl::cbuffer_view message);
    uint32_t get_msg_method_idx_req(cl::cbuffer_view message);

    uint64_t get_msg_number_res(cl::cbuffer_view message);
    response_result get_msg_response_code_res(cl::cbuffer_view message);

    cl::buffer create_transfer_msg_req(uint64_t message_number,
                                       uint32_t method_idx,
                                       const google::protobuf::Message *proto_message);

    cl::buffer create_transfer_msg_res(uint64_t message_number,
                                       response_result rc,
                                       google::protobuf::Message *proto_message);

    cl::buffer create_transfer_msg_ping();
    cl::buffer create_transfer_msg_pong();
}
