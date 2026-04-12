#pragma once
#include "rpc-lib/types/request-result.h"
#include "rpc-lib/types/response-result.h"

#include <common-lib/utils/buffer-view/cbuffer-view.h>
#include <common-lib/utils/buffer/buffer.h>

#pragma warning(push, 0)
#include <google/protobuf/message.h>
#pragma warning(pop)

#include <string>

/*
    A transfer_msg is a single, logically complete message transmitted
    between one process and another. The structure consists of three parts: a header,
    a serialized message, and a trailer.

    Each transfer_msg has its own type:
    transfer_type::req - request sent from one process to an another
    transfer_type::res - response sent from one process to an another as an answer to the request

    Structure of all types is the same header:
    1 byte message type + 4 bytes size of serialize message

    Every type has its own trailer:
    req: 8 bytes message number + 4 bytes method idx
    res: 8 bytes message number + 1 byte response result code
*/

namespace vshalygin::rpc {
    enum class transfer_msg_type : unsigned char
    {
        req = 0,
        res = 1
    };

    bool is_transfer_msg_too_big(const google::protobuf::Message *proto_message);

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
}
