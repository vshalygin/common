#pragma once
#include "rpc-lib/types/request-result.h"
#include "rpc-lib/types/response-result.h"

#include <common-lib/utils/cbuffer-view.h>
#include <common-lib/utils/buffer.h>

#include <string>

/*
    A transfer_msg is a single, logically complete message transmitted
    between a client and a server.
    Each transfer_msg has its own type:
    transfer_type::req  - a request sent from one endpoint to another
    transfer_type::res  - a response sent from one endpoint to another in reply to a request
    transfer_type::ping - a special request sent to check connection validity
    transfer_type::pong - a response to a ping

    The req and res types have the same header:
    1 byte for the message type + 4 bytes for the serialized Protobuf message size

    The req and res types have their own trailers:
    req: 8 bytes for the message number + 4 bytes for the method index
    res: 8 bytes for the message number + 1 byte for the response result code

    Ping and pong messages have at least one byte at the beginning for the
    message type; the remaining bytes are unspecified.
*/

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
