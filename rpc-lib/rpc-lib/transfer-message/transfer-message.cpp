//#include "transfer-message.h"
//#include <cassert>
//#include <array>
//
//namespace vsh::rpc {
//    namespace {
//        constexpr const unsigned s_bits_in_byte = 8;
//
//        constexpr const unsigned s_message_type_bytes_num = 1;
//        constexpr const unsigned s_serialized_message_size_bytes_num = 4;
//
//        constexpr const unsigned s_message_number_bytes_num = 8;
//        constexpr const unsigned s_client_id_bytes_num = 32;
//        constexpr const unsigned s_method_idx_bytes_num = 4;
//
//        constexpr const unsigned s_header_bytes_num = s_message_type_bytes_num + 
//                                                      s_serialized_message_size_bytes_num;
//
//        constexpr const unsigned s_req_trailer_bytes_num = s_message_number_bytes_num +
//                                                           s_client_id_bytes_num +
//                                                           s_method_idx_bytes_num;
//
//        constexpr const unsigned s_res_trailer_bytes_num = s_message_number_bytes_num;
//
//        unsigned to_unsigned_big_endian(cl::cbuffer_view bytes)
//        {
//            assert(bytes.size() <= sizeof(unsigned));
//
//            unsigned ans = 0;
//            for(size_t i = 0; i < bytes.size(); ++i) {
//                unsigned offset = static_cast<unsigned>(bytes.size() - i - 1) * s_bits_in_byte;
//                unsigned add_bits = static_cast<unsigned>(bytes[i]) << offset;
//                ans |= add_bits;
//            }
//
//            return ans;
//        }
//
//        uint64_t to_uint64_big_endian(cl::cbuffer_view bytes)
//        {
//            assert(bytes.size() <= sizeof(uint64_t));
//
//            uint64_t ans = 0;
//            for(size_t i = 0; i < bytes.size(); ++i) {
//                uint64_t offset = (bytes.size() - i - 1) * s_bits_in_byte;
//                uint64_t add_bits = static_cast<uint64_t>(bytes[i]) << offset;
//                ans |= add_bits;
//            }
//
//            return ans;
//        }
//
//        std::array<unsigned char, 4> from_unsigned_big_endian(unsigned number)
//        {
//            std::array<unsigned char, 4> ans;
//
//            const auto max_size = static_cast<unsigned>(ans.max_size());
//            for(unsigned i = 0; i < max_size; ++i) {
//                unsigned offset = (max_size - i - 1) * s_bits_in_byte;
//                auto byte = static_cast<unsigned char>((number >> offset) & 0xFF);
//                ans[i] = byte;
//            }
//
//            return ans;
//        }
//
//        std::array<unsigned char, 8> from_uint64_big_endian(uint64_t number)
//        {
//            std::array<unsigned char, 8> ans;
//
//            const auto max_size = static_cast<unsigned>(ans.max_size());
//            for(unsigned i = 0; i < max_size; ++i) {
//                unsigned offset = (max_size - i - 1) * s_bits_in_byte;
//                auto byte = static_cast<unsigned char>((number >> offset) & 0xFF);
//                ans[i] = byte;
//            }
//
//            return ans;
//        }
//
//        unsigned extract_message_size(cl::cbuffer_view message)
//        {
//            const auto begin = message.data() + s_message_type_bytes_num;
//
//            return to_unsigned_big_endian(cl::cbuffer_view{ begin, s_serialized_message_size_bytes_num });
//        }
//
//        void fill_message_type_byte(cl::buffer &buff, size_t &pos, message_type type)
//        {
//            buff[pos++] = static_cast<unsigned char>(type);
//        }
//        
//        void fill_serialized_message_size_bytes(cl::buffer &buff,
//                                                size_t &pos,
//                                                unsigned serialized_message_size)
//        {
//            auto serialized_message_size_bytes = from_unsigned_big_endian(serialized_message_size);
//            assert(serialized_message_size_bytes.size() == s_serialized_message_size_bytes_num);
//            for(int i = 0; i < serialized_message_size_bytes.size(); ++i) {
//                buff[pos++] = serialized_message_size_bytes[i];
//            }
//        }
//
//        void fill_serialized_message_bytes(cl::buffer &buff,
//                                           size_t &pos,
//                                           const google::protobuf::Message *message,
//                                           unsigned serialized_message_size)
//        {
//            //TODO check if failed
//            message->SerializeToArray(buff.data() + pos, static_cast<unsigned>(buff.size() - pos));
//            pos += serialized_message_size;
//        }
//
//        void fill_message_number_bytes(cl::buffer &buff,
//                                     size_t &pos,
//                                     uint64_t message_number)
//        {
//            auto message_number_bytes = from_uint64_big_endian(message_number);
//            assert(message_number_bytes.size() == s_message_number_bytes_num);
//            for(int i = 0; i < message_number_bytes.size(); ++i) {
//                buff[pos++] = message_number_bytes[i];
//            }
//        }
//
//        void fill_client_id_bytes(cl::buffer &buff,
//                                  size_t &pos,
//                                  const std::string &client_id)
//        {
//            assert(client_id.size() == s_client_id_bytes_num);
//            for(size_t i = 0; i < client_id.size(); ++i) {
//                buff[pos++] = static_cast<unsigned char>(client_id[i]);
//            }
//        }
//
//        void fill_method_idx_bytes(cl::buffer &buff,
//                                   size_t &pos,
//                                   unsigned method_idx)
//        {
//            auto method_idx_bytes = from_unsigned_big_endian(method_idx);
//            assert(method_idx_bytes.size() == s_method_idx_bytes_num);
//            for(int i = 0; i < method_idx_bytes.size(); ++i) {
//                buff[pos++] = method_idx_bytes[i];
//            }
//        }
//
//        void assert_message_req([[maybe_unused]] const cl::cbuffer_view &message)
//        {
//            assert(message.data());
//            assert(message.size() >= s_header_bytes_num);
//            assert(s_header_bytes_num + extract_message_size(message) +
//                   s_req_trailer_bytes_num <= message.size());
//        }
//
//        void assert_message_res([[maybe_unused]] const cl::cbuffer_view &message)
//        {
//            assert(message.data());
//            assert(message.size() >= s_header_bytes_num);
//            assert(s_header_bytes_num +
//                   extract_message_size(message) +
//                   s_res_trailer_bytes_num <= message.size());
//        }
//    }
//
//    message_type get_transfer_message_type(cl::cbuffer_view message)
//    {
//        assert(message.data());
//        assert(message.size() > 0);
//
//        return static_cast<message_type>(message[0]);
//    }
//
//    cl::cbuffer_view get_serialized_message(cl::cbuffer_view message)
//    {
//        assert(message.data());
//        assert(s_header_bytes_num <= message.size());
//        assert(s_header_bytes_num + extract_message_size(message) <= message.size());
//
//        auto begin = message.data() + s_header_bytes_num;
//        return cl::cbuffer_view(begin, extract_message_size(message));
//    }
//
//    uint64_t get_message_number_req(cl::cbuffer_view message)
//    {
//        assert_message_req(message);
//
//        auto begin = message.data() + s_header_bytes_num + extract_message_size(message);
//        return to_uint64_big_endian(cl::cbuffer_view{ begin, s_message_number_bytes_num });
//    }
//
//    cl::cbuffer_view get_client_id_req(cl::cbuffer_view message)
//    {
//        assert_message_req(message);
//
//        auto begin = message.data() + s_header_bytes_num +
//                     extract_message_size(message) +
//                     s_message_number_bytes_num;
//        return cl::cbuffer_view(begin, s_client_id_bytes_num);
//    }
//
//    unsigned get_method_idx_req(cl::cbuffer_view message)
//    {
//        assert_message_req(message);
//
//        auto begin = message.data() + s_header_bytes_num +
//                                    extract_message_size(message) +
//                                    s_message_number_bytes_num +
//                                    s_client_id_bytes_num;
//
//        return to_unsigned_big_endian(cl::cbuffer_view{ begin, s_method_idx_bytes_num });
//    }
//
//    uint64_t get_message_number_res(cl::cbuffer_view message)
//    {
//        assert_message_res(message);
//
//        auto begin = message.data() + s_header_bytes_num + extract_message_size(message);
//        return to_uint64_big_endian({ begin, s_message_number_bytes_num });
//    }
//
//    cl::buffer create_transfer_message_req(uint64_t message_number,
//                                           const std::string &client_id,
//                                           unsigned method_idx,
//                                           const google::protobuf::Message *message)
//    {
//        assert(message);
//
//        const unsigned serialized_message_size = static_cast<unsigned>(message->ByteSizeLong());
//        const unsigned buf_size = s_header_bytes_num + serialized_message_size + s_req_trailer_bytes_num;
//
//        cl::buffer ans(buf_size);
//        size_t curr_pos = 0;
//
//        fill_message_type_byte(ans, curr_pos, message_type::req);
//        fill_serialized_message_size_bytes(ans, curr_pos, serialized_message_size);
//        fill_serialized_message_bytes(ans, curr_pos, message, serialized_message_size);
//        fill_message_number_bytes(ans, curr_pos, message_number);
//        fill_client_id_bytes(ans, curr_pos, client_id);
//        fill_method_idx_bytes(ans, curr_pos, method_idx);
//
//        assert(curr_pos == buf_size);
//        return ans;
//    }
//
//    cl::buffer create_transfer_message_res(uint64_t message_number,
//                                           google::protobuf::Message *message)
//    {
//        assert(message);
//
//        const unsigned serialized_message_size = static_cast<unsigned>(message->ByteSizeLong());
//        const unsigned buf_size = s_header_bytes_num + serialized_message_size + s_res_trailer_bytes_num;
//
//        cl::buffer ans(buf_size);
//        size_t curr_pos = 0;
//
//        fill_message_type_byte(ans, curr_pos, message_type::res);
//        fill_serialized_message_size_bytes(ans, curr_pos, serialized_message_size);
//        fill_serialized_message_bytes(ans, curr_pos, message, serialized_message_size);
//        fill_message_number_bytes(ans, curr_pos, message_number);
//
//        assert(curr_pos == buf_size);
//        return ans;
//    }
//}
