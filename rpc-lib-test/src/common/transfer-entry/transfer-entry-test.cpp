#include <rpc-lib/common/transfer-entry/transfer-entry.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>

using namespace vsh::rpc;
using namespace testing;

namespace {
    constexpr const unsigned s_entry_number = 4291937406; //0xFFD1C47E
    const std::vector<char> s_entry_number_bytes = { '\xFF', '\xD1', '\xC4', '\x7E' };

    const std::string s_client_id = "2E92794A409548B1994B54CD9CECFBA1";

    constexpr const unsigned s_method_idx = 4091900406; //0xF3E571F6
    const std::vector<unsigned char> s_method_idx_bytes = { 0xF3, 0xE5, 0x71, 0xF6 };

    const std::string s_serialized_message = "3l;ed45631"; //some rubbish
    const std::vector<char> s_serialized_message_size_str = { 0x00, 0x00, 0x00, 0x0A };

    std::vector<unsigned char> create_entry()
    {
        std::vector<unsigned char> res;
        res.push_back(static_cast<char>(transfer_type::req));
        res.insert(res.cend(), s_serialized_message_size_str.begin(), s_serialized_message_size_str.end());
        res.insert(res.cend(), s_serialized_message.begin(), s_serialized_message.end());
        res.insert(res.cend(), s_entry_number_bytes.begin(), s_entry_number_bytes.end());
        res.insert(res.cend(), s_client_id.begin(), s_client_id.end());
        res.insert(res.cend(), s_method_idx_bytes.begin(), s_method_idx_bytes.end());

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
    const auto entry = create_entry();

    ASSERT_EQ(transfer_type::req, get_transfer_entry_type(entry.data(), entry.size()));
}

TEST(TransferEntryReq, ResolveEntryNumber)
{
    const auto entry = create_entry();

    ASSERT_EQ(s_entry_number, transfer_view_req(entry.data(), entry.size()).get_entry_number_req());
}

TEST(TransferEntryReq, ResolveClientId)
{
    const auto entry = create_entry();

    auto client_id = transfer_view_req(entry.data(), entry.size()).get_client_id_req();
    ASSERT_THAT(reinterpret_cast<const unsigned char *>(s_client_id.data()),
                ArrayEq(client_id.data(), client_id.size()));
}

TEST(TransferEntryReq, ResolveMethodIdx)
{
    const auto entry = create_entry();

    ASSERT_EQ(s_method_idx, transfer_view_req(entry.data(), entry.size()).get_method_idx_req());
}

TEST(TransferEntryReq, ResolveSerializedMessage)
{
    const auto entry = create_entry();

    auto serialize_message = transfer_view_req(entry.data(), entry.size()).get_serialized_message_req();
    ASSERT_THAT(reinterpret_cast<const unsigned char *>(s_serialized_message.data()),
                ArrayEq(serialize_message.data(), serialize_message.size()));
}
