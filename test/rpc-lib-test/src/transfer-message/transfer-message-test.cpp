#include <rpc-lib/internal/transfer-message/transfer-message.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#pragma warning(pop)

#include <string>

using namespace vshalygin::rpc;
using namespace vshalygin::rpc::internal;
using namespace vshalygin::cl;

using namespace testing;

namespace {
    proto::some_message get_test_message()
    {
        proto::some_message msg;
        msg.set_some_data(34);

        return msg;
    }

    void set_max_serialized_message_size(buffer &message)
    {
        message[1] = static_cast<std::byte>(0xFF);
        message[2] = static_cast<std::byte>(0xFF);
        message[3] = static_cast<std::byte>(0xFF);
        message[4] = static_cast<std::byte>(0xFF);
    }

    constexpr const uint64_t s_entry_number = 429193740634345; //0x18659747348E9
    const std::vector<std::byte> s_entry_number_bytes = { (std::byte)0x00,
                                                          (std::byte)0x01,
                                                          (std::byte)0x86,
                                                          (std::byte)0x59,
                                                          (std::byte)0x74,
                                                          (std::byte)0x73,
                                                          (std::byte)0x48,
                                                          (std::byte)0xE9 };

    constexpr const unsigned s_method_idx = 4091900406; //0xF3E571F6
    const std::vector<std::byte> s_method_idx_bytes = { (std::byte)0xF3, (std::byte)0xE5,
                                                        (std::byte)0x71, (std::byte)0xF6 };
    const response_result s_response_code = response_result::unknown_error;

    std::vector<std::byte> to_bytes(const std::string &str)
    {
        std::vector<std::byte> ans;
        for(size_t i = 0; i < str.size(); ++i) {
            unsigned offset = 0x0;
            for(size_t j = 0; j < sizeof(std::string::value_type); ++j) {
                ans.push_back((std::byte)((str[i] >> offset) & 0xFF));
                offset += 8;
            }
        }
        return ans;
    }

    std::vector<std::byte> create_transfer_msg_req()
    {
        auto test_msg = get_test_message();
        auto serialized_test_msg = test_msg.SerializeAsString();
        auto serialized_test_msg_bytes = to_bytes(serialized_test_msg);
        
        auto serialized_test_msg_size = static_cast<unsigned>(serialized_test_msg.size());
        std::vector<std::byte> serialized_message_size_bytes{ (std::byte)(serialized_test_msg_size >> 24),
                                                              (std::byte)(serialized_test_msg_size >> 16),
                                                              (std::byte)(serialized_test_msg_size >> 8),
                                                              (std::byte)(serialized_test_msg_size >> 0) };
        std::vector<std::byte> res;
        res.push_back(static_cast<std::byte>(transfer_msg_type::req));
        res.insert(res.cend(), serialized_message_size_bytes.begin(), serialized_message_size_bytes.end());
        res.insert(res.cend(), serialized_test_msg_bytes.begin(), serialized_test_msg_bytes.end());
        res.insert(res.cend(), s_entry_number_bytes.begin(), s_entry_number_bytes.end());
        res.insert(res.cend(), s_method_idx_bytes.begin(), s_method_idx_bytes.end());

        return res;
    }

    std::vector<std::byte> create_transfer_msg_res()
    {
        auto test_msg = get_test_message();
        auto serialized_test_msg = test_msg.SerializeAsString();
        auto serialized_test_msg_bytes = to_bytes(serialized_test_msg);

        auto serialized_test_msg_size = static_cast<unsigned>(serialized_test_msg.size());
        std::vector<std::byte> serialized_message_size_bytes{ (std::byte)(serialized_test_msg_size >> 24),
                                                              (std::byte)(serialized_test_msg_size >> 16),
                                                              (std::byte)(serialized_test_msg_size >> 8),
                                                              (std::byte)(serialized_test_msg_size >> 0) };
        std::vector<std::byte> res;
        res.push_back(static_cast<std::byte>(transfer_msg_type::res));
        res.insert(res.cend(), serialized_message_size_bytes.begin(), serialized_message_size_bytes.end());
        res.insert(res.cend(), serialized_test_msg_bytes.begin(), serialized_test_msg_bytes.end());
        res.insert(res.cend(), s_entry_number_bytes.begin(), s_entry_number_bytes.end());
        res.push_back(static_cast<std::byte>(s_response_code));
        return res;
    }

    //TODO move to test lib
    MATCHER_P2(ArrayEq, expected, size, "Arrays are equal") {
        for(size_t i = 0; i < size; ++i) {
            if(arg[i] != expected[i]) {
                return false;
            }
        }
        return true;
    }
}

TEST(TransferMessageReq, IsRequestProtoTooBigReturnFalseIfMessageNotTooBig)
{
    proto::some_message test_message;
    test_message.set_string_data("small string");

    ASSERT_FALSE(is_request_proto_too_big(&test_message));
}

TEST(TransferMessageReq, IsResponseProtoTooBigReturnFalseIfMessageNotTooBig)
{
    proto::some_message test_message;
    test_message.set_string_data("small string");

    ASSERT_FALSE(is_response_proto_too_big(&test_message));
}

TEST(TransferMessageReq, IsRequestProtoTooBigReturnTrueIfMessageTooBig)
{
    proto::some_message test_message;
    test_message.set_string_data(std::string(8*1024*1024 + 1001, 'a'));

    ASSERT_TRUE(is_request_proto_too_big(&test_message));
}

TEST(TransferMessageReq, IsResponseProtoTooBigReturnTrueIfMessageTooBig)
{
    proto::some_message test_message;
    test_message.set_string_data(std::string(8 * 1024 * 1024 + 1001, 'a'));

    ASSERT_TRUE(is_response_proto_too_big(&test_message));
}

TEST(TransferMessageReq, ResolveMessageType)
{
    const auto entry = create_transfer_msg_req();

    ASSERT_EQ(transfer_msg_type::req, get_transfer_msg_type({ entry.data(), entry.size()} ));
}

TEST(TransferMessageReq, ResolveMessageNumber)
{
    const auto entry = create_transfer_msg_req();

    ASSERT_EQ(s_entry_number, get_msg_number_req({ entry.data(), entry.size() }));
}

TEST(TransferMessageReq, ResolveMethodIdx)
{
    const auto entry = create_transfer_msg_req();

    ASSERT_EQ(s_method_idx, get_msg_method_idx_req({ entry.data(), entry.size() }));
}

TEST(TransferMessageReq, ResolveSerializedMessage)
{
    const auto entry = create_transfer_msg_req();
    const auto expected_serialized_message = get_test_message().SerializeAsString();
    auto expected_serialized_message_bytes = to_bytes(expected_serialized_message);

    auto serialize_message = get_serialized_proto_message({ entry.data(), entry.size() });
    ASSERT_THAT(expected_serialized_message_bytes.data(),
                ArrayEq(serialize_message.data(), serialize_message.size()));
}

TEST(TransferMessageReq, CreatesTransferMessage)
{
    const auto expected_entry = create_transfer_msg_req();
    const auto expected_entry_view = cbuffer_view{ expected_entry.data(), expected_entry.size() };
    proto::some_message test_message;
    test_message.ParseFromArray(get_serialized_proto_message(expected_entry_view).data(),
                                (int)get_serialized_proto_message(expected_entry_view).size());

    auto entry = create_transfer_msg_req(get_msg_number_req(expected_entry_view),
                                         get_msg_method_idx_req(expected_entry_view),
                                         &test_message);

    ASSERT_THAT(expected_entry.data(),
                ArrayEq(entry.data(), entry.size()));
}

TEST(TransferMessageReq, CreatesTransferMessageWithNullptrMessage)
{
    auto entry = create_transfer_msg_req(34,
                                         8,
                                         nullptr);

    ASSERT_EQ(get_serialized_proto_message(entry).size(), 0);
}

TEST(TransferMessageReq, ThrowsExceptionOnCreateTransferMessageIsMessageTwoBig)
{
    proto::some_message test_message;
    test_message.set_string_data(std::string(1024 * 1024 + 1, 'a'));

    ASSERT_ANY_THROW(create_transfer_msg_req(1, 1, &test_message));
}

TEST(TransferMessageReq, GetTransferMsgTypeThrowsExceptionOnInvalidBuffer)
{
    ASSERT_ANY_THROW(get_transfer_msg_type(buffer{}));
    ASSERT_ANY_THROW(get_transfer_msg_type(buffer{ 0 }));
}

TEST(TransferMessageReq, GetSerializedProtoMessageThrowsExceptionOnInvalidBuffer)
{
    ASSERT_ANY_THROW(get_serialized_proto_message(buffer{}));
    ASSERT_ANY_THROW(get_serialized_proto_message(buffer{ 1 }));

    proto::some_message test_message;
    test_message.set_string_data(std::string(10, 'a'));
    auto b = create_transfer_msg_req(1, 1, &test_message);
    ASSERT_ANY_THROW(get_serialized_proto_message(cbuffer_view{ b.data(), b.size() - test_message.ByteSizeLong() - 1}));
}

TEST(TransferMessageReq, GetSerializedProtoMessageRejectsUint32MaxPayloadSize)
{
    auto b = create_transfer_msg_req(1, 1, nullptr);
    set_max_serialized_message_size(b);

    EXPECT_ANY_THROW(get_serialized_proto_message(b));
}

TEST(TransferMessageReq, GetMsgNumberReqThrowsExceptionOnInvalidBuffer)
{
    ASSERT_ANY_THROW(get_msg_number_req(buffer{}));
    ASSERT_ANY_THROW(get_msg_number_req(buffer{ 1 }));
}

TEST(TransferMessageReq, GetMsgMethodIdxReqThrowsExceptionOnInvalidBuffer)
{
    ASSERT_ANY_THROW(get_msg_method_idx_req(buffer{}));
    ASSERT_ANY_THROW(get_msg_method_idx_req(buffer{ 1 }));
}

TEST(TransferMessageReq, TestIsRequestBufferValid)
{
    proto::some_message test_message; test_message.set_string_data(std::string(100, ' '));
    buffer b1;
    buffer b2(3);
    buffer b3 = create_transfer_msg_res(1, response_result::ok, &test_message);
    buffer b4 = create_transfer_msg_req(1, 1, nullptr);

    EXPECT_FALSE(is_request_buffer_valid(b1));
    EXPECT_FALSE(is_request_buffer_valid(b2));
    EXPECT_FALSE(is_request_buffer_valid(b3));
    EXPECT_FALSE(is_request_buffer_valid(cbuffer_view(b4.data(), b4.size() - 1)));
    EXPECT_TRUE(is_request_buffer_valid(b4));
}

TEST(TransferMessageReq, RejectsUint32MaxPayloadSize)
{
    auto b = create_transfer_msg_req(1, 1, nullptr);
    set_max_serialized_message_size(b);

    EXPECT_FALSE(is_request_buffer_valid(b));
    EXPECT_ANY_THROW(get_msg_number_req(b));
    EXPECT_ANY_THROW(get_msg_method_idx_req(b));
}

TEST(TransferMessageRes, ResolveMessageNumber)
{
    const auto entry = create_transfer_msg_res();

    ASSERT_EQ(s_entry_number, get_msg_number_res({ entry.data(), entry.size() }));
}

TEST(TransferMessageRes, ResolveResponseCode)
{
    const auto entry = create_transfer_msg_res();

    ASSERT_EQ(s_response_code, get_msg_response_code_res({ entry.data(), entry.size() }));
}

TEST(TransferMessageRes, ResolveSerializedMessage)
{
    const auto entry = create_transfer_msg_res();
    const auto expected_serialized_message = get_test_message().SerializeAsString();
    const auto expected_serialized_message_bytes = to_bytes(expected_serialized_message);

    auto serialize_message = get_serialized_proto_message({ entry.data(), entry.size() });
    ASSERT_THAT(expected_serialized_message_bytes.data(),
                ArrayEq(serialize_message.data(), serialize_message.size()));
}

TEST(TransferMessageRes, CreatesTransferMessage)
{
    const auto expected_entry = create_transfer_msg_res();
    const auto expected_entry_view = cbuffer_view{ expected_entry.data(), expected_entry.size() };
    proto::some_message test_message;
    test_message.ParseFromArray(get_serialized_proto_message(expected_entry_view).data(),
                                (int)get_serialized_proto_message(expected_entry_view).size());

    auto entry = create_transfer_msg_res(get_msg_number_res(expected_entry_view),
                                         get_msg_response_code_res(expected_entry_view),
                                         &test_message);

    ASSERT_THAT(expected_entry.data(),
                ArrayEq(entry.data(), entry.size()));
}

TEST(TransferMessageRes, ThrowsExceptionOnCreateTransferMessageIsMessageTwoBig)
{
    proto::some_message test_message;
    test_message.set_string_data(std::string(8 * 1024 * 1024 + 1, 'a'));

    ASSERT_ANY_THROW(create_transfer_msg_res(1, response_result::ok, &test_message));
}

TEST(TransferMessageRes, CreatesTransferMessageWithNullptrMessage)
{
    auto entry = create_transfer_msg_res(34,
                                         response_result::not_implemented,
                                         nullptr);

    ASSERT_EQ(get_serialized_proto_message(entry).size(), 0);
}

TEST(TransferMessageReq, TestIsResultBufferValid)
{
    proto::some_message test_message; test_message.set_string_data(std::string(100, ' '));
    buffer b1;
    buffer b2(3);
    buffer b3 = create_transfer_msg_req(1, 1, &test_message);
    buffer b4 = create_transfer_msg_res(34, response_result::not_implemented, nullptr);

    EXPECT_FALSE(is_response_buffer_valid(b1));
    EXPECT_FALSE(is_response_buffer_valid(b2));
    EXPECT_FALSE(is_response_buffer_valid(b3));
    EXPECT_FALSE(is_response_buffer_valid(cbuffer_view(b4.data(), b4.size() - 1)));
    EXPECT_TRUE(is_response_buffer_valid(b4));
}

TEST(TransferMessageRes, RejectsUint32MaxPayloadSize)
{
    auto b = create_transfer_msg_res(1, response_result::ok, nullptr);
    set_max_serialized_message_size(b);

    EXPECT_FALSE(is_response_buffer_valid(b));
    EXPECT_ANY_THROW(get_msg_number_res(b));
    EXPECT_ANY_THROW(get_msg_response_code_res(b));
}

TEST(TransferMessageReq, GetMsgResponseCodeEesThrowsExceptionOnInvalidBuffer)
{
    ASSERT_ANY_THROW(get_msg_response_code_res(buffer{}));
    ASSERT_ANY_THROW(get_msg_response_code_res(buffer{ 1 }));
}

TEST(TransferMessageReq, GetMsgNumberResThrowsExceptionOnInvalidBuffer)
{
    ASSERT_ANY_THROW(get_msg_number_res(buffer{}));
    ASSERT_ANY_THROW(get_msg_number_res(buffer{ 1 }));
}

TEST(TransferMessage, TestCreateTransferMessagePing)
{
    auto msg = create_transfer_msg_ping();

    ASSERT_EQ(msg.size(), 1u);
    EXPECT_EQ(static_cast<transfer_msg_type>(msg[0]), transfer_msg_type::ping);
}

TEST(TransferMessage, TestCreateTransferMessagePong)
{
    auto msg = create_transfer_msg_pong();

    ASSERT_EQ(msg.size(), 1u);
    EXPECT_EQ(static_cast<transfer_msg_type>(msg[0]), transfer_msg_type::pong);
}
