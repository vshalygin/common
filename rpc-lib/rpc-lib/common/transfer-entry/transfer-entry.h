#pragma once
#include <string>

/*
    A transfer_entry is a single, logically complete message transmitted
    between the server and the client. he structure consists of three parts: a header,
    a serialized message, and a trailer.

    Each transfer_entry has its own type:
    transfer_type::req - entry sent from client to server
    transfer_type::res - entry sent from server to client as an answer to the request

    Structure of all types has one structure header:
    1 byte entry type + 4 bytes size of serialize message

    Every type has its own trailer:
    req: 4 bytes entry number + 32 bytes client id + 4 bytes method idx
    res: 4 bytes entry number + 32 bytes client id + 4 bytes method idx
*/

namespace vsh::rpc {
    enum class transfer_type : char
    {
        req = 0,
        res = 1
    };

    transfer_type get_entry_type(const char *buf, size_t size);

    class transfer_view_req
    {
    public:
        transfer_view_req(const char *buf, size_t size);

        unsigned get_entry_number_req() const;
        std::string_view get_client_id_req() const;
        unsigned get_method_idx_req() const;
        std::string_view get_serialized_message_req() const;

    private:
        const char *buf_;
        size_t size_;

        unsigned message_size_;
    };

    using transfer_view_res = transfer_view_req;
}
