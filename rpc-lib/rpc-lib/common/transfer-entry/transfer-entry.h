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
    transfer_type::req - entry sent from client to server
    transfer_type::res - entry sent from server to client as an answer to the request

    Structure of all types is the same header:
    1 byte entry type + 4 bytes size of serialize message

    Every type has its own trailer:
    req: 4 bytes entry number + 32 bytes client id + 4 bytes method idx
    res: 4 bytes entry number
*/

namespace vsh::rpc {
    enum class transfer_type : unsigned char
    {
        req = 0,
        res = 1
    };

    transfer_type get_transfer_entry_type(common_lib::cbuffer_view entry);

    unsigned get_entry_number_req(common_lib::cbuffer_view entry);
    common_lib::cbuffer_view get_client_id_req(common_lib::cbuffer_view entry);
    unsigned get_method_idx_req(common_lib::cbuffer_view entry);
    common_lib::cbuffer_view get_serialized_message_req(common_lib::cbuffer_view entry);

    unsigned get_entry_number_res(common_lib::cbuffer_view entry);
    common_lib::cbuffer_view get_serialized_message_res(common_lib::cbuffer_view entry);

    common_lib::buffer create_transfer_entry_req(unsigned entry_number,
                                                 const std::string &client_id,
                                                 unsigned method_idx,
                                                 const google::protobuf::Message *message);

    common_lib::buffer create_transfer_entry_res(unsigned entry_number,
                                                 google::protobuf::Message *message);
}
