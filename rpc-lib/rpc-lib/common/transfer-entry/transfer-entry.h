#pragma once
#include <common-lib/utils/buffer-view/cbuffer-view.h>
#include <common-lib/utils/buffer/buffer.h>

#pragma warning(push, 0)
#include <google/protobuf/message.h>
#pragma warning(pop)

#include <string>

/*
    A transfer_entry is a single, logically complete message transmitted
    between the server and the client. he structure consists of three parts: a header,
    a serialized message, and a trailer.

    Each transfer_entry has its own type:
    transfer_type::req - request sent from the client to the server
    transfer_type::res - response sent from the server to the client as an answer to the request
    transfer_type::evt - event sent from the server to the client
    transfer_type::ack - acknowledgement sent from the client to the server as an answer to the event

    Structure of all types is the same header:
    1 byte entry type + 4 bytes size of serialize message

    Every type has its own trailer:
    req: 8 bytes entry number + 32 bytes client id + 4 bytes method idx
    res: 8 bytes entry number
    evt: 8 bytes entry number + 4 bytes method idx
    ack: 8 bytes entry number + 32 bytes client id
*/

namespace vsh::rpc {
    enum class transfer_type : unsigned char
    {
        req = 0,
        res = 1,
        evt = 2,
        ack = 3
    };

    transfer_type get_transfer_entry_type(cl::cbuffer_view entry);
    cl::cbuffer_view get_serialized_message(cl::cbuffer_view entry);

    uint64_t get_entry_number_req(cl::cbuffer_view entry);
    cl::cbuffer_view get_client_id_req(cl::cbuffer_view entry);
    unsigned get_method_idx_req(cl::cbuffer_view entry);

    uint64_t get_entry_number_res(cl::cbuffer_view entry);
    
    uint64_t get_entry_number_evt(cl::cbuffer_view entry);
    unsigned get_method_idx_evt(cl::cbuffer_view entry);

    uint64_t get_entry_number_ack(cl::cbuffer_view entry);
    cl::cbuffer_view get_client_id_ack(cl::cbuffer_view entry);

    cl::buffer create_transfer_entry_req(uint64_t entry_number,
                                         const std::string &client_id,
                                         unsigned method_idx,
                                         const google::protobuf::Message *message);

    cl::buffer create_transfer_entry_res(uint64_t entry_number,
                                         google::protobuf::Message *message);

    cl::buffer create_transfer_entry_evt(uint64_t entry_number,
                                         unsigned method_idx,
                                         google::protobuf::Message *message);

    cl::buffer create_transfer_entry_ack(uint64_t entry_number,
                                         const std::string &client_id,
                                         const google::protobuf::Message *message);
}
