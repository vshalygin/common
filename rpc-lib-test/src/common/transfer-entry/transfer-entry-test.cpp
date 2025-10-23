#include <rpc-lib/common/transfer-entry/transfer-entry.h>

#include <gtest/gtest.h>

#include <string>

using namespace vsh::rpc;
using namespace testing;

namespace {
    constexpr const unsigned entry_number = 4291937406; //0xFFD1C47E
    const std::vector<char> entry_number_bytes = { '\xFF', '\xD1', '\xC4', '\x7E' };

    const std::string client_id = "2E92794A409548B1994B54CD9CECFBA1";

    constexpr const unsigned method_idx = 4091900406; //0xF3E571F6
    const std::vector<char> method_idx_bytes = { '\xF3', '\xE5', '\x71', '\xF6' };

    const std::string serialized_message = "3l;ed45631"; //some rubbish
    const std::vector<char> serialized_message_size_str = { '\x00', '\x00', '\x00', '\x0A' };

    std::vector<char> create_entry()
    {
        std::vector<char> res;
        res.push_back(static_cast<char>(transfer_type::req));
        res.insert(res.cend(), serialized_message_size_str.begin(), serialized_message_size_str.end());
        res.insert(res.cend(), serialized_message.begin(), serialized_message.end());
        res.insert(res.cend(), entry_number_bytes.begin(), entry_number_bytes.end());
        res.insert(res.cend(), client_id.begin(), client_id.end());
        res.insert(res.cend(), method_idx_bytes.begin(), method_idx_bytes.end());

        return res;
    }
}

TEST(TransferEntryReq, ResolveEntryType)
{
    const auto entry = create_entry();

    ASSERT_EQ(transfer_type::req, get_entry_type(entry.data(), entry.size()));
}

TEST(TransferEntryReq, ResolveEntryNumber)
{
    const auto entry = create_entry();

    ASSERT_EQ(entry_number, transfer_view_req(entry.data(), entry.size()).get_entry_number_req());
}

TEST(TransferEntryReq, ResolveClientId)
{
    const auto entry = create_entry();

    ASSERT_EQ(client_id, transfer_view_req(entry.data(), entry.size()).get_client_id_req());
}

TEST(TransferEntryReq, ResolveMethodIdx)
{
    const auto entry = create_entry();

    ASSERT_EQ(method_idx, transfer_view_req(entry.data(), entry.size()).get_method_idx_req());
}

TEST(TransferEntryReq, ResolveSerializedMessage)
{
    const auto entry = create_entry();

    ASSERT_EQ(serialized_message, transfer_view_req(entry.data(), entry.size()).get_serialized_message_req());
}
