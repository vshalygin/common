#include "transfer-entry.h"
#include <cassert>

/*
    A transfer_entry is a single, logically complete message transmitted
    between the server and the client. It consists of a header and a serialized
    protobuf message.

    Each transfer_entry has its own type:
    transfer_type::req - entry sent from client to server
    transfer_type::res - entry sent from server to client as an answer to the request

    Each type has its own header structure:
    req: 1 byte entry type + 4 bytes entry id + 32 bytes client id + 4 bytes method idx
    res: 1 byte entry type + 4 bytes entry id + 32 bytes client id + 4 bytes method idx
*/

namespace vsh::rpc {
    namespace {
        constexpr const unsigned entry_type_bytes = 1;
        constexpr const unsigned entry_id_bytes = 4;
        constexpr const unsigned client_id_bytes = 32;
        constexpr const unsigned method_idx_bytes = 4;

        constexpr const unsigned req_header_size = entry_type_bytes +
                                                   entry_id_bytes +
                                                   client_id_bytes +
                                                   method_idx_bytes;
        constexpr const unsigned res_header_size = req_header_size;

        unsigned get_header_size(transfer_type type)
        {
            switch(type) {
                case transfer_type::req:
                    return req_header_size;
                case transfer_type::res:
                    return res_header_size;
                default:
                    assert(!"unknown transfer type for 'get_header_size'");
                    return 0;
            }
        }

        void assert_prerequests([[maybe_unused]] const char *buf,
                                [[maybe_unused]] size_t size,
                                [[maybe_unused]] transfer_type type)
        {
            assert(buf);
            assert(size >= get_header_size(type));
            assert(static_cast<transfer_type>(buf[0]) == type);
        }

        unsigned to_unsigned_big_endian(const char *buf, unsigned size)
        {
            unsigned ans = 0;
            for(unsigned i = 0; i < size; ++i) {
                ans += static_cast<unsigned>(buf[i]) << ((size-i-1) * sizeof(char));
            }

            return ans;
        }
    }

    unsigned get_entry_id_req(const char *buf, size_t size)
    {
        assert_prerequests(buf, size, transfer_type::req);

        const char *begin = buf + entry_type_bytes;

        return to_unsigned_big_endian(begin, entry_id_bytes);
    }

    std::string_view get_client_id_req(const char *buf, size_t size)
    {
        assert_prerequests(buf, size, transfer_type::req);

        const char *begin = buf + entry_type_bytes + entry_id_bytes;

        return std::string_view(begin, client_id_bytes);;
    }

    unsigned get_method_idx_req(const char *buf, size_t size)
    {
        assert_prerequests(buf, size, transfer_type::req);

        const char *begin = buf + entry_type_bytes + entry_id_bytes + client_id_bytes;

        return to_unsigned_big_endian(begin, method_idx_bytes);
    }

    std::string_view get_serialized_message_req(const char *buf, size_t size)
    {
        assert_prerequests(buf, size, transfer_type::req);

        const char *begin = buf + req_header_size;

        return std::string_view(begin, size - req_header_size);
    }
}