#include "transfer-entry.h"
#include <cassert>
#include <array>

namespace vsh::rpc {
    namespace {
        constexpr const unsigned s_bits_in_byte = 8;

        constexpr const unsigned s_entry_type_bytes_num = 1;
        constexpr const unsigned s_entry_message_size_bytes_num = 4;

        constexpr const unsigned s_entry_number_bytes_num = 4;
        constexpr const unsigned s_client_id_bytes_num = 32;
        constexpr const unsigned s_method_idx_bytes_num = 4;

        constexpr const unsigned s_header_bytes_num = s_entry_type_bytes_num + s_entry_message_size_bytes_num;

        constexpr const unsigned s_req_trailer_bytes_num = s_entry_number_bytes_num +
                                                           s_client_id_bytes_num +
                                                           s_method_idx_bytes_num;
        constexpr const unsigned s_res_trailer_bytes_num = s_req_trailer_bytes_num;

        unsigned to_unsigned_big_endian(const unsigned char *buf, unsigned size)
        {
            unsigned ans = 0;
            for(unsigned i = 0; i < size; ++i) {
                unsigned offset = (size - i - 1) * s_bits_in_byte;
                unsigned add_bits = static_cast<unsigned>(buf[i]) << offset;
                ans |= add_bits;
            }

            return ans;
        }

        std::array<unsigned char, 4> from_unsigned_big_endian(unsigned number)
        {
            std::array<unsigned char, 4> ans;

            const auto max_size = static_cast<unsigned>(ans.max_size());
            for(unsigned i = 0; i < max_size; ++i) {
                unsigned offset = (max_size - i - 1) * s_bits_in_byte;
                auto byte = static_cast<unsigned char>((number >> offset) & 0xFF);
                ans[i] = byte;
            }

            return ans;
        }

        unsigned extract_message_size(const unsigned char *buf)
        {
            const auto begin = buf + s_entry_type_bytes_num;

            return to_unsigned_big_endian(begin, s_entry_message_size_bytes_num);
        }
    }

    transfer_type get_transfer_entry_type(const unsigned char *buf,
                                          [[maybe_unused]] size_t size)
    {
        assert(buf);
        assert(size > 0);

        return static_cast<transfer_type>(*buf);
    }

    transfer_view_req::transfer_view_req(const unsigned char *buf, size_t size)
        : buf_(buf)
        , size_(size)
    {
        assert(buf);
        assert(size >= s_header_bytes_num);

        message_size_ = extract_message_size(buf);
        assert(s_header_bytes_num + message_size_ + s_req_trailer_bytes_num <= size);
    }

    unsigned transfer_view_req::get_entry_number() const
    {
        auto begin = buf_ + s_header_bytes_num + message_size_;

        return to_unsigned_big_endian(begin, s_entry_number_bytes_num);
    }

    common_lib::cbuffer_view transfer_view_req::get_client_id() const
    {
        auto begin = buf_ + s_header_bytes_num + message_size_ + s_entry_number_bytes_num;

        return common_lib::cbuffer_view(begin, s_client_id_bytes_num);
    }

    unsigned transfer_view_req::get_method_idx() const
    {
        auto begin = buf_ + s_header_bytes_num +
                     message_size_ +
                     s_entry_number_bytes_num +
                     s_client_id_bytes_num;

        return to_unsigned_big_endian(begin, s_method_idx_bytes_num);
    }

    common_lib::cbuffer_view transfer_view_req::get_serialized_message() const
    {
        auto begin = buf_ + s_header_bytes_num;

        return common_lib::cbuffer_view(begin, message_size_);
    }

    common_lib::buffer create_transfer_entry_req(unsigned entry_number,
                                                 const std::string &client_id,
                                                 unsigned method_idx,
                                                 google::protobuf::Message *message)
    {
        assert(message);

        unsigned serialized_message_size = static_cast<unsigned>(message->ByteSizeLong());
        unsigned buf_size = s_header_bytes_num + serialized_message_size + s_req_trailer_bytes_num;

        common_lib::buffer ans(buf_size);
        auto curr_pos = ans.data();
        unsigned filled_bytes = 0;

        //fill entry type byte
        *(curr_pos++) = static_cast<unsigned char>(transfer_type::req);
        ++filled_bytes;

        //fill serialized message size bytes
        auto serialized_message_size_bytes = from_unsigned_big_endian(serialized_message_size);
        assert(serialized_message_size_bytes.size() == s_entry_message_size_bytes_num);
        for(int i = 0; i < serialized_message_size_bytes.size(); ++i) {
            *(curr_pos++) = serialized_message_size_bytes[i];
            ++filled_bytes;
        }

        //fill serialized message bytes
        message->SerializeToArray(curr_pos, buf_size - filled_bytes); //TODO check if failed
        curr_pos += serialized_message_size;
        filled_bytes += serialized_message_size;

        //fill entry number bytes
        auto entry_number_bytes = from_unsigned_big_endian(entry_number);
        assert(entry_number_bytes.size() == s_entry_number_bytes_num);
        for(int i = 0; i < entry_number_bytes.size(); ++i) {
            *(curr_pos++) = entry_number_bytes[i];
            ++filled_bytes;
        }

        //fill client id bytes
        assert(client_id.size() == s_client_id_bytes_num);
        for(size_t i = 0; i < client_id.size(); ++i) {
            *(curr_pos++) = static_cast<unsigned char>(client_id[i]);
            ++filled_bytes;
        }

        //fill method idx bytes
        auto method_idx_bytes = from_unsigned_big_endian(method_idx);
        assert(method_idx_bytes.size() == s_method_idx_bytes_num);
        for(int i = 0; i < method_idx_bytes.size(); ++i) {
            *(curr_pos++) = method_idx_bytes[i];
            ++filled_bytes;
        }

        assert(filled_bytes == buf_size);
        return ans;
    }

    common_lib::buffer create_transfer_entry_res(unsigned entry_number,
                                                 const std::string &client_id,
                                                 unsigned method_id,
                                                 google::protobuf::Message *message)
    {
        return create_transfer_entry_req(entry_number, client_id, method_id, message);
    }
}
