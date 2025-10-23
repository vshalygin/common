#include "transfer-entry.h"
#include <cassert>

namespace vsh::rpc {
    namespace {
        constexpr const unsigned bits_in_byte = 8;

        constexpr const unsigned entry_type_bytes = 1;
        constexpr const unsigned entry_message_size_bytes = 4;

        constexpr const unsigned entry_number_bytes = 4;
        constexpr const unsigned client_id_bytes = 32;
        constexpr const unsigned method_idx_bytes = 4;

        constexpr const unsigned header_size = entry_type_bytes + entry_message_size_bytes;

        constexpr const unsigned req_trailer_size = entry_number_bytes +
                                                    client_id_bytes +
                                                    method_idx_bytes;
        constexpr const unsigned res_trailer_size = req_trailer_size;

        unsigned to_unsigned_big_endian(const char *buf, unsigned size)
        {
            unsigned ans = 0;
            for(unsigned i = 0; i < size; ++i) {
                unsigned offset = (size - i - 1) * bits_in_byte;
                auto byte = static_cast<unsigned char>(buf[i]);
                unsigned add_bits = static_cast<unsigned>(byte) << offset;
                ans |= add_bits;
            }

            return ans;
        }

        unsigned extract_message_size(const char *buf)
        {
            const char *begin = buf + entry_type_bytes;

            return to_unsigned_big_endian(begin, entry_message_size_bytes);
        }
    }

    transfer_type get_entry_type(const char *buf, [[maybe_unused]] size_t size)
    {
        assert(buf);
        assert(size > 0);

        return static_cast<transfer_type>(*buf);
    }

    transfer_view_req::transfer_view_req(const char *buf, size_t size)
        : buf_(buf)
        , size_(size)
    {
        assert(buf);
        assert(size >= header_size);

        message_size_ = extract_message_size(buf);
        assert(header_size + message_size_ + req_trailer_size <= size);
    }

    unsigned transfer_view_req::get_entry_number_req() const
    {
        const char *begin = buf_ + header_size + message_size_;

        return to_unsigned_big_endian(begin, entry_number_bytes);
    }

    std::string_view transfer_view_req::get_client_id_req() const
    {
        const char *begin = buf_ + header_size + message_size_ + entry_number_bytes;

        return std::string_view(begin, client_id_bytes);
    }

    unsigned transfer_view_req::get_method_idx_req() const
    {
        const char *begin = buf_ + header_size + message_size_ + entry_number_bytes + client_id_bytes;

        return to_unsigned_big_endian(begin, method_idx_bytes);
    }

    std::string_view transfer_view_req::get_serialized_message_req() const
    {
        const char *begin = buf_ + header_size;

        return std::string_view(begin, message_size_);
    }
}
