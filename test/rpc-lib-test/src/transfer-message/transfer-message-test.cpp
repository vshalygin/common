#include <rpc-lib/transfer-message/transfer-message.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "proto/test-messages.pb.h"

#include <string>

using namespace vsh::rpc;
using namespace vsh::cl;

using namespace testing;

namespace {
    proto::some_message get_test_message()
    {
        proto::some_message msg;
        msg.set_some_data(34);

        return msg;
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
    const response_result s_response_code = response_result::rejected;

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

TEST(TransferEntryReq, ResolveEntryType)
{
    const auto entry = create_transfer_msg_req();

    ASSERT_EQ(transfer_msg_type::req, get_transfer_msg_type({ entry.data(), entry.size()} ));
}

TEST(TransferEntryReq, ResolveEntryNumber)
{
    const auto entry = create_transfer_msg_req();

    ASSERT_EQ(s_entry_number, get_msg_number_req({ entry.data(), entry.size() }));
}

TEST(TransferEntryReq, ResolveMethodIdx)
{
    const auto entry = create_transfer_msg_req();

    ASSERT_EQ(s_method_idx, get_msg_method_idx_req({ entry.data(), entry.size() }));
}

TEST(TransferEntryReq, ResolveSerializedMessage)
{
    const auto entry = create_transfer_msg_req();
    const auto expected_serialized_message = get_test_message().SerializeAsString();
    auto expected_serialized_message_bytes = to_bytes(expected_serialized_message);

    auto serialize_message = get_serialized_proto_message({ entry.data(), entry.size() });
    ASSERT_THAT(expected_serialized_message_bytes.data(),
                ArrayEq(serialize_message.data(), serialize_message.size()));
}

TEST(TransferEntryReq, CreateTransferEntry)
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

TEST(TransferEntryRes, ResolveEntryNumber)
{
    const auto entry = create_transfer_msg_res();

    ASSERT_EQ(s_entry_number, get_msg_number_res({ entry.data(), entry.size() }));
}

TEST(TransferEntryRes, ResolveResponseCode)
{
    const auto entry = create_transfer_msg_res();

    ASSERT_EQ(s_response_code, get_msg_response_code_res({ entry.data(), entry.size() }));
}

TEST(TransferEntryRes, ResolveSerializedMessage)
{
    const auto entry = create_transfer_msg_res();
    const auto expected_serialized_message = get_test_message().SerializeAsString();
    const auto expected_serialized_message_bytes = to_bytes(expected_serialized_message);

    auto serialize_message = get_serialized_proto_message({ entry.data(), entry.size() });
    ASSERT_THAT(expected_serialized_message_bytes.data(),
                ArrayEq(serialize_message.data(), serialize_message.size()));
}

TEST(TransferEntryRes, CreateTransferEntry)
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
