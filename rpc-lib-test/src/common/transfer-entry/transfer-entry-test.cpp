#include <rpc-lib/common/transfer-entry/transfer-entry.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "proto/test-messages.pb.h"

#include <string>

using namespace vsh::rpc;
using namespace vsh::common_lib;

using namespace testing;

namespace {
    proto::some_message get_test_message()
    {
        proto::some_message msg;
        msg.set_some_data(34);

        return msg;
    }

    constexpr const unsigned s_entry_number = 4291937406; //0xFFD1C47E
    const std::vector<char> s_entry_number_bytes = { '\xFF', '\xD1', '\xC4', '\x7E' };

    const std::string s_client_id = "2E92794A409548B1994B54CD9CECFBA1";

    constexpr const unsigned s_method_idx = 4091900406; //0xF3E571F6
    const std::vector<unsigned char> s_method_idx_bytes = { 0xF3, 0xE5, 0x71, 0xF6 };

    std::vector<unsigned char> create_entry_req()
    {
        auto test_msg = get_test_message();
        auto serialized_test_msg = test_msg.SerializeAsString();
        auto serialized_test_msg_size = static_cast<unsigned>(serialized_test_msg.size());
        std::vector<unsigned char> serialized_message_size_bytes{ (unsigned char)(serialized_test_msg_size >> 24),
                                                                  (unsigned char)(serialized_test_msg_size >> 16),
                                                                  (unsigned char)(serialized_test_msg_size >> 8),
                                                                  (unsigned char)(serialized_test_msg_size >> 0) };
        std::vector<unsigned char> res;
        res.push_back(static_cast<char>(transfer_type::req));
        res.insert(res.cend(), serialized_message_size_bytes.begin(), serialized_message_size_bytes.end());
        res.insert(res.cend(), serialized_test_msg.begin(), serialized_test_msg.end());
        res.insert(res.cend(), s_entry_number_bytes.begin(), s_entry_number_bytes.end());
        res.insert(res.cend(), s_client_id.begin(), s_client_id.end());
        res.insert(res.cend(), s_method_idx_bytes.begin(), s_method_idx_bytes.end());

        return res;
    }

    std::vector<unsigned char> create_entry_res()
    {
        auto test_msg = get_test_message();
        auto serialized_test_msg = test_msg.SerializeAsString();
        auto serialized_test_msg_size = static_cast<unsigned>(serialized_test_msg.size());
        std::vector<unsigned char> serialized_message_size_bytes{ (unsigned char)(serialized_test_msg_size >> 24),
                                                                  (unsigned char)(serialized_test_msg_size >> 16),
                                                                  (unsigned char)(serialized_test_msg_size >> 8),
                                                                  (unsigned char)(serialized_test_msg_size >> 0) };
        std::vector<unsigned char> res;
        res.push_back(static_cast<char>(transfer_type::res));
        res.insert(res.cend(), serialized_message_size_bytes.begin(), serialized_message_size_bytes.end());
        res.insert(res.cend(), serialized_test_msg.begin(), serialized_test_msg.end());
        res.insert(res.cend(), s_entry_number_bytes.begin(), s_entry_number_bytes.end());

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
    const auto entry = create_entry_req();

    ASSERT_EQ(transfer_type::req, get_transfer_entry_type({ entry.data(), entry.size()} ));
}

TEST(TransferEntryReq, ResolveEntryNumber)
{
    const auto entry = create_entry_req();

    ASSERT_EQ(s_entry_number, get_entry_number_req({ entry.data(), entry.size() }));
}

TEST(TransferEntryReq, ResolveClientId)
{
    const auto entry = create_entry_req();

    auto client_id = get_client_id_req({ entry.data(), entry.size() });
    ASSERT_THAT(reinterpret_cast<const unsigned char *>(s_client_id.data()),
                ArrayEq(client_id.data(), client_id.size()));
}

TEST(TransferEntryReq, ResolveMethodIdx)
{
    const auto entry = create_entry_req();

    ASSERT_EQ(s_method_idx, get_method_idx_req({ entry.data(), entry.size() }));
}

TEST(TransferEntryReq, ResolveSerializedMessage)
{
    const auto entry = create_entry_req();
    const auto expected_serialized_message = get_test_message().SerializeAsString();

    auto serialize_message = get_serialized_message_req({ entry.data(), entry.size() });
    ASSERT_THAT(reinterpret_cast<const unsigned char *>(expected_serialized_message.data()),
                ArrayEq(serialize_message.data(), serialize_message.size()));
}

TEST(TransferEntryReq, CreateTransferEntry)
{
    const auto expected_entry = create_entry_req();
    const auto expected_entry_view = cbuffer_view{ expected_entry.data(), expected_entry.size() };
    proto::some_message test_message;
    test_message.ParseFromArray(get_serialized_message_req(expected_entry_view).data(),
                                get_serialized_message_req(expected_entry_view).size());
    std::string client_id(reinterpret_cast<const char *>(get_client_id_req(expected_entry_view).data()),
                          get_client_id_req(expected_entry_view).size());

    auto entry = create_transfer_entry_req(get_entry_number_req(expected_entry_view),
                                           client_id,
                                           get_method_idx_req(expected_entry_view),
                                           &test_message);

    ASSERT_THAT(expected_entry.data(),
                ArrayEq(entry.data(), entry.size()));
}

TEST(TransferEntryRes, ResolveEntryNumber)
{
    const auto entry = create_entry_res();

    ASSERT_EQ(s_entry_number, get_entry_number_res({ entry.data(), entry.size() }));
}

TEST(TransferEntryRes, ResolveSerializedMessage)
{
    const auto entry = create_entry_res();
    const auto expected_serialized_message = get_test_message().SerializeAsString();

    auto serialize_message = get_serialized_message_res({ entry.data(), entry.size() });
    ASSERT_THAT(reinterpret_cast<const unsigned char *>(expected_serialized_message.data()),
                ArrayEq(serialize_message.data(), serialize_message.size()));
}

TEST(TransferEntryRes, CreateTransferEntry)
{
    const auto expected_entry = create_entry_res();
    const auto expected_entry_view = cbuffer_view{ expected_entry.data(), expected_entry.size() };
    proto::some_message test_message;
    test_message.ParseFromArray(get_serialized_message_res(expected_entry_view).data(),
                                get_serialized_message_res(expected_entry_view).size());

    auto entry = create_transfer_entry_res(get_entry_number_res(expected_entry_view),
                                           &test_message);

    ASSERT_THAT(expected_entry.data(),
                ArrayEq(entry.data(), entry.size()));
}
