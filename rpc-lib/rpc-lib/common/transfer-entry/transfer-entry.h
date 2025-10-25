#pragma once
#include "rpc-lib/common/buffer/buffer.h"
#include "rpc-lib/common/buffer-view/cbuffer-view.h"

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

    Structure of all types is the same:
    1 byte entry type + 4 bytes size of serialize message

    Every type has its own trailer:
    req: 4 bytes entry number + 32 bytes client id + 4 bytes method idx
    res: 4 bytes entry number + 32 bytes client id + 4 bytes method idx
*/

namespace vsh::rpc {
    enum class transfer_type : unsigned char
    {
        req = 0,
        res = 1
    };

    transfer_type get_transfer_entry_type(const unsigned char *buf, size_t size);

    class transfer_view_req
    {
    public:
        transfer_view_req(const unsigned char *buf, size_t size);

        unsigned get_entry_number() const;
        cbuffer_view get_client_id() const;
        unsigned get_method_idx() const;
        cbuffer_view get_serialized_message() const;

    private:
        const unsigned char *buf_;
        size_t size_;

        unsigned message_size_;
    };

    using transfer_view_res = transfer_view_req;

    buffer create_transfer_entry_req(unsigned entry_number,
                                     const std::string &client_id,
                                     unsigned method_idx,
                                     google::protobuf::Message *message);

    buffer create_transfer_entry_res(unsigned entry_number,
                                     const std::string &client_id,
                                     unsigned method_idx,
                                     google::protobuf::Message *message);
}
