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

        constexpr const unsigned s_res_trailer_bytes_num = s_entry_number_bytes_num;

        unsigned to_unsigned_big_endian(common_lib::cbuffer_view bytes)
        {
            unsigned ans = 0;
            for(size_t i = 0; i < bytes.size(); ++i) {
                unsigned offset = static_cast<unsigned>(bytes.size() - i - 1) * s_bits_in_byte;
                unsigned add_bits = static_cast<unsigned>(bytes[i]) << offset;
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

        unsigned extract_message_size(common_lib::cbuffer_view entry)
        {
            const auto begin = entry.data() + s_entry_type_bytes_num;

            return to_unsigned_big_endian(common_lib::cbuffer_view{ begin, s_entry_message_size_bytes_num });
        }

        void fill_entry_type_byte(common_lib::buffer &buff, size_t &pos, transfer_type type)
        {
            buff[pos++] = static_cast<unsigned char>(type);
        }
        
        void fill_serialized_message_size_bytes(common_lib::buffer &buff,
                                                size_t &pos,
                                                unsigned serialized_message_size)
        {
            auto serialized_message_size_bytes = from_unsigned_big_endian(serialized_message_size);
            assert(serialized_message_size_bytes.size() == s_entry_message_size_bytes_num);
            for(int i = 0; i < serialized_message_size_bytes.size(); ++i) {
                buff[pos++] = serialized_message_size_bytes[i];
            }
        }

        void fill_serialized_message_bytes(common_lib::buffer &buff,
                                           size_t &pos,
                                           google::protobuf::Message *message,
                                           unsigned serialized_message_size)
        {
            //TODO check if failed
            message->SerializeToArray(buff.data() + pos, static_cast<unsigned>(buff.size() - pos));
            pos += serialized_message_size;
        }

        void fill_entry_number_bytes(common_lib::buffer &buff,
                                     size_t &pos,
                                     unsigned entry_number)
        {
            auto entry_number_bytes = from_unsigned_big_endian(entry_number);
            assert(entry_number_bytes.size() == s_entry_number_bytes_num);
            for(int i = 0; i < entry_number_bytes.size(); ++i) {
                buff[pos++] = entry_number_bytes[i];
            }
        }

        void fill_client_id_bytes(common_lib::buffer &buff,
                                  size_t &pos,
                                  const std::string &client_id)
        {
            assert(client_id.size() == s_client_id_bytes_num);
            for(size_t i = 0; i < client_id.size(); ++i) {
                buff[pos++] = static_cast<unsigned char>(client_id[i]);
            }
        }

        void fill_method_idx_bytes(common_lib::buffer &buff,
                                   size_t &pos,
                                   unsigned method_idx)
        {
            auto method_idx_bytes = from_unsigned_big_endian(method_idx);
            assert(method_idx_bytes.size() == s_method_idx_bytes_num);
            for(int i = 0; i < method_idx_bytes.size(); ++i) {
                buff[pos++] = method_idx_bytes[i];
            }
        }

        void assert_entry_req([[maybe_unused]] const common_lib::cbuffer_view &entry)
        {
            assert(entry.data());
            assert(entry.size() >= s_header_bytes_num);
            assert(s_header_bytes_num + extract_message_size(entry) + s_req_trailer_bytes_num <= entry.size());
        }

        void assert_entry_res([[maybe_unused]] const common_lib::cbuffer_view &entry)
        {
            assert(entry.data());
            assert(entry.size() >= s_header_bytes_num);
            assert(s_header_bytes_num + extract_message_size(entry) + s_res_trailer_bytes_num <= entry.size());
        }
    }

    transfer_type get_transfer_entry_type(common_lib::cbuffer_view entry)
    {
        assert(entry.data());
        assert(entry.size() > 0);

        return static_cast<transfer_type>(entry[0]);
    }

    unsigned get_entry_number_req(common_lib::cbuffer_view entry)
    {
        assert_entry_req(entry);

        auto begin = entry.data() + s_header_bytes_num + extract_message_size(entry);
        return to_unsigned_big_endian(common_lib::cbuffer_view{ begin, s_entry_number_bytes_num });
    }

    common_lib::cbuffer_view get_client_id_req(common_lib::cbuffer_view entry)
    {
        assert_entry_req(entry);

        auto begin = entry.data() + s_header_bytes_num + extract_message_size(entry) + s_entry_number_bytes_num;
        return common_lib::cbuffer_view(begin, s_client_id_bytes_num);
    }

    unsigned get_method_idx_req(common_lib::cbuffer_view entry)
    {
        assert_entry_req(entry);

        auto begin = entry.data() + s_header_bytes_num +
                                    extract_message_size(entry) +
                                    s_entry_number_bytes_num +
                                    s_client_id_bytes_num;

        return to_unsigned_big_endian(common_lib::cbuffer_view{ begin, s_method_idx_bytes_num });
    }

    common_lib::cbuffer_view get_serialized_message_req(common_lib::cbuffer_view entry)
    {
        assert_entry_req(entry);

        auto begin = entry.data() + s_header_bytes_num;
        return common_lib::cbuffer_view(begin, extract_message_size(entry));
    }

    unsigned get_entry_number_res(common_lib::cbuffer_view entry)
    {
        assert_entry_res(entry);

        auto begin = entry.data() + s_header_bytes_num + extract_message_size(entry);
        return to_unsigned_big_endian(common_lib::cbuffer_view{ begin, s_entry_number_bytes_num });
    }

    common_lib::cbuffer_view get_serialized_message_res(common_lib::cbuffer_view entry)
    {
        auto begin = entry.data() + s_header_bytes_num;
        return common_lib::cbuffer_view(begin, extract_message_size(entry));
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
        size_t curr_pos = 0;

        fill_entry_type_byte(ans, curr_pos, transfer_type::req);
        fill_serialized_message_size_bytes(ans, curr_pos, serialized_message_size);
        fill_serialized_message_bytes(ans, curr_pos, message, serialized_message_size);
        fill_entry_number_bytes(ans, curr_pos, entry_number);
        fill_client_id_bytes(ans, curr_pos, client_id);
        fill_method_idx_bytes(ans, curr_pos, method_idx);

        assert(curr_pos == buf_size);
        return ans;
    }

    common_lib::buffer create_transfer_entry_res(unsigned entry_number,
                                                 google::protobuf::Message *message)
    {
        assert(message);

        unsigned serialized_message_size = static_cast<unsigned>(message->ByteSizeLong());
        unsigned buf_size = s_header_bytes_num + serialized_message_size + s_res_trailer_bytes_num;

        common_lib::buffer ans(buf_size);
        size_t curr_pos = 0;

        fill_entry_type_byte(ans, curr_pos, transfer_type::res);
        fill_serialized_message_size_bytes(ans, curr_pos, serialized_message_size);
        fill_serialized_message_bytes(ans, curr_pos, message, serialized_message_size);
        fill_entry_number_bytes(ans, curr_pos, entry_number);

        assert(curr_pos == buf_size);
        return ans;
    }
}
