#include "transfer-message.h"
#include <rpc-lib/consts.h>

#pragma warning(push, 0)
#include <google/protobuf/message.h>
#pragma warning(pop)

#include <cassert>
#include <array>

namespace vshalygin::rpc::internal {
    namespace {
        constexpr const unsigned s_bits_in_byte = 8;

        constexpr const unsigned s_message_type_bytes_count = 1;
        constexpr const unsigned s_serialized_proto_message_size_bytes_count = 4;

        constexpr const unsigned s_message_number_bytes_count = 8;
        constexpr const unsigned s_method_idx_bytes_count = 4;
        constexpr const unsigned s_result_code_bytes_count = 1;

        constexpr const unsigned s_header_bytes_count = s_message_type_bytes_count +
                                                        s_serialized_proto_message_size_bytes_count;

        constexpr const unsigned s_req_trailer_bytes_count = s_message_number_bytes_count +
                                                             s_method_idx_bytes_count;

        constexpr const unsigned s_res_trailer_bytes_count = s_message_number_bytes_count +
                                                             s_result_code_bytes_count;

        static_assert(MaxTransferMessageSize > MaxRequestProtoSize + s_header_bytes_count + s_req_trailer_bytes_count);
        static_assert(MaxTransferMessageSize > MaxResponseProtoSize + s_header_bytes_count + s_res_trailer_bytes_count);

        uint32_t to_uint32_to_big_endian(cl::cbuffer_view bytes) noexcept
        {
            assert(bytes.size() == sizeof(uint32_t));

            uint32_t ans = 0;
            for(size_t i = 0; i < bytes.size(); ++i) {
                uint32_t offset = static_cast<uint32_t>(bytes.size() - i - 1) * s_bits_in_byte;
                uint32_t add_bits = static_cast<uint32_t>(bytes[i]) << offset;
                ans |= add_bits;
            }

            return ans;
        }

        uint64_t to_uint64_big_endian(cl::cbuffer_view bytes)
        {
            assert(bytes.size() == sizeof(uint64_t));

            uint64_t ans = 0;
            for(size_t i = 0; i < bytes.size(); ++i) {
                uint64_t offset = (bytes.size() - i - 1) * s_bits_in_byte;
                uint64_t add_bits = static_cast<uint64_t>(bytes[i]) << offset;
                ans |= add_bits;
            }

            return ans;
        }

        std::array<std::byte, sizeof(uint32_t)> from_uint32_to_big_endian(uint32_t number)
        {
            std::array<std::byte, sizeof(uint32_t)> ans;

            const auto max_size = static_cast<uint32_t>(ans.max_size());
            for(uint32_t i = 0; i < max_size; ++i) {
                uint32_t offset = (max_size - i - 1) * s_bits_in_byte;
                auto byte = static_cast<std::byte>((number >> offset) & 0xFF);
                ans[i] = byte;
            }

            return ans;
        }

        std::array<std::byte, 8> from_uint64_big_endian(uint64_t number)
        {
            std::array<std::byte, 8> ans;

            const auto max_size = static_cast<uint32_t>(ans.max_size());
            for(uint32_t i = 0; i < max_size; ++i) {
                uint32_t offset = (max_size - i - 1) * s_bits_in_byte;
                auto byte = static_cast<std::byte>((number >> offset) & 0xFF);
                ans[i] = byte;
            }

            return ans;
        }

        uint32_t extract_message_size(cl::cbuffer_view message) noexcept
        {
            const auto begin = message.data() + s_message_type_bytes_count;

            return to_uint32_to_big_endian(
                cl::cbuffer_view{ begin, s_serialized_proto_message_size_bytes_count });
        }

        void fill_message_type_byte(cl::buffer &buff, size_t &pos, transfer_msg_type type)
        {
            buff[pos++] = static_cast<std::byte>(type);
        }
        
        void fill_serialized_message_size_bytes(cl::buffer &buff,
                                                size_t &pos,
                                                uint32_t serialized_message_size)
        {
            auto serialized_message_size_bytes = from_uint32_to_big_endian(serialized_message_size);
            assert(serialized_message_size_bytes.size() == s_serialized_proto_message_size_bytes_count);
            for(int i = 0; i < serialized_message_size_bytes.size(); ++i) {
                buff[pos++] = serialized_message_size_bytes[i];
            }
        }

        void fill_serialized_message_bytes(cl::buffer &buff,
                                           size_t &pos,
                                           const google::protobuf::Message *message,
                                           uint32_t serialized_message_size)
        {
            if(!message) {
                return;
            }

            assert(buff.size() - pos >= serialized_message_size);
            if(!message->SerializeToArray(buff.data() + pos, static_cast<int>(buff.size() - pos))) {
                throw std::runtime_error("failed to serialize message");
            }
            pos += serialized_message_size;
        }

        void fill_message_number_bytes(cl::buffer &buff,
                                     size_t &pos,
                                     uint64_t message_number)
        {
            auto message_number_bytes = from_uint64_big_endian(message_number);
            assert(message_number_bytes.size() == s_message_number_bytes_count);
            for(int i = 0; i < message_number_bytes.size(); ++i) {
                buff[pos++] = message_number_bytes[i];
            }
        }

        void fill_method_idx_bytes(cl::buffer &buff,
                                   size_t &pos,
                                   uint32_t method_idx)
        {
            auto method_idx_bytes = from_uint32_to_big_endian(method_idx);
            assert(method_idx_bytes.size() == s_method_idx_bytes_count);
            for(int i = 0; i < method_idx_bytes.size(); ++i) {
                buff[pos++] = method_idx_bytes[i];
            }
        }

        void fill_response_result_bytes(cl::buffer &buff,
                                        size_t &pos,
                                        response_result rc)
        {
            buff[pos++] = static_cast<std::byte>(rc);
        }
    }

    bool is_request_buffer_valid(cl::cbuffer_view buff) noexcept
    {
        return buff.data() && buff.size() >= s_header_bytes_count + s_req_trailer_bytes_count &&
               extract_message_size(buff) <= buff.size() - s_header_bytes_count - s_req_trailer_bytes_count;
    }

    bool is_response_buffer_valid(cl::cbuffer_view buff) noexcept
    {
        return buff.data() && buff.size() >= s_header_bytes_count + s_res_trailer_bytes_count &&
               extract_message_size(buff) <= buff.size() - s_header_bytes_count - s_res_trailer_bytes_count;
    }

    bool is_request_proto_too_big(const google::protobuf::Message *proto_message)
    {
        const auto serialized_message_size = proto_message->ByteSizeLong();
        return (serialized_message_size > MaxRequestProtoSize);
    }

    bool is_response_proto_too_big(const google::protobuf::Message *proto_message)
    {
        const auto serialized_message_size = proto_message->ByteSizeLong();
        return (serialized_message_size > MaxResponseProtoSize);
    }

    transfer_msg_type get_transfer_msg_type(cl::cbuffer_view message)
    {
        if(!message.data() || message.size() == 0) {
            throw std::runtime_error("invalid buffer");
        }

        return static_cast<transfer_msg_type>(message[0]);
    }

    cl::cbuffer_view get_serialized_proto_message(cl::cbuffer_view message)
    {
        if(!message.data() ||
           s_header_bytes_count > message.size() ||
           extract_message_size(message) > message.size() - s_header_bytes_count)
        {
            throw std::runtime_error("invalid buffer");
        }

        auto begin = message.data() + s_header_bytes_count;
        return cl::cbuffer_view(begin, extract_message_size(message));
    }

    uint64_t get_msg_number_req(cl::cbuffer_view message)
    {
        if(!is_request_buffer_valid(message)) {
            throw std::runtime_error("invalid buffer");
        }

        auto begin = message.data() + s_header_bytes_count + extract_message_size(message);
        return to_uint64_big_endian(cl::cbuffer_view{ begin, s_message_number_bytes_count });
    }

    uint32_t get_msg_method_idx_req(cl::cbuffer_view message)
    {
        if(!is_request_buffer_valid(message)) {
            throw std::runtime_error("invalid buffer");
        }

        auto begin = message.data() + s_header_bytes_count +
                                      extract_message_size(message) +
                                      s_message_number_bytes_count;

        return to_uint32_to_big_endian(cl::cbuffer_view{ begin, s_method_idx_bytes_count });
    }

    uint64_t get_msg_number_res(cl::cbuffer_view message)
    {
        if(!is_response_buffer_valid(message)) {
            throw std::runtime_error("invalid buffer");
        }

        auto begin = message.data() + s_header_bytes_count + extract_message_size(message);
        return to_uint64_big_endian({ begin, s_message_number_bytes_count });
    }

    response_result get_msg_response_code_res(cl::cbuffer_view message)
    {
        if(!is_response_buffer_valid(message)) {
            throw std::runtime_error("invalid buffer");
        }

        auto iter = message.data() +
                    s_header_bytes_count +
                    extract_message_size(message) +
                    s_message_number_bytes_count;

        return static_cast<response_result>(*iter);
    }

    cl::buffer create_transfer_msg_req(uint64_t message_number,
                                       uint32_t method_idx,
                                       const google::protobuf::Message *message)
    {
        size_t serialized_message_size = 0;
        if(message) {
            serialized_message_size = message->ByteSizeLong();
        }

        if(serialized_message_size > MaxRequestProtoSize) {
            throw std::runtime_error("message is too big");
        }

        const uint32_t buf_size = s_header_bytes_count +
                                  static_cast<uint32_t>(serialized_message_size) +
                                  s_req_trailer_bytes_count;

        cl::buffer ans(buf_size);
        size_t curr_pos = 0;

        fill_message_type_byte(ans, curr_pos, transfer_msg_type::req);
        fill_serialized_message_size_bytes(ans, curr_pos, static_cast<uint32_t>(serialized_message_size));
        fill_serialized_message_bytes(ans, curr_pos, message, static_cast<uint32_t>(serialized_message_size));
        fill_message_number_bytes(ans, curr_pos, message_number);
        fill_method_idx_bytes(ans, curr_pos, method_idx);

        assert(curr_pos == buf_size);
        return ans;
    }

    cl::buffer create_transfer_msg_res(uint64_t message_number,
                                       response_result rc,
                                       google::protobuf::Message *message)
    {
        size_t serialized_message_size = 0;
        if(message) {
            serialized_message_size = message->ByteSizeLong();
        }

        if(serialized_message_size > MaxResponseProtoSize) {
            throw std::runtime_error("message is too big");
        }

        const uint32_t buf_size = s_header_bytes_count +
                                  static_cast<uint32_t>(serialized_message_size) +
                                  s_res_trailer_bytes_count;

        cl::buffer ans(buf_size);
        size_t curr_pos = 0;

        fill_message_type_byte(ans, curr_pos, transfer_msg_type::res);
        fill_serialized_message_size_bytes(ans, curr_pos, static_cast<uint32_t>(serialized_message_size));
        fill_serialized_message_bytes(ans, curr_pos, message, static_cast<uint32_t>(serialized_message_size));
        fill_message_number_bytes(ans, curr_pos, message_number);
        fill_response_result_bytes(ans, curr_pos, rc);

        assert(curr_pos == buf_size);
        return ans;
    }
}
