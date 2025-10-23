#include <rpc-lib/common/transfer-entry/transfer-entry.h>

#include <gtest/gtest.h>

#include <string>

using namespace vsh::rpc;
using namespace testing;

namespace {
    constexpr const unsigned entry_number = 4291937406; //0xFFD1C47E
    const std::string entry_number_bytes = "\xFF\xD1\xC4\x7E";

    const std::string client_id = "2E92794A409548B1994B54CD9CECFBA1";

    constexpr const unsigned method_idx = 4091900406; //0xF3E571F6
    const std::string method_idx_bytes = "\xF3\xE5\x71\xF6";

    const std::string serialized_message = "3l;ewrj;le342;4jrf0sdje0jfj02js0d"; //some rubbish

    std::string create_entry()
    {
        std::string res;
        res += static_cast<char>(transfer_type::req);
        res += entry_number_bytes;
        res += client_id;
        res += method_idx_bytes;
        res += serialized_message;

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

    ASSERT_EQ(entry_number, get_entry_number_req(entry.data(), entry.size()));
}

TEST(TransferEntryReq, ResolveClientId)
{
    const auto entry = create_entry();

    ASSERT_EQ(client_id, get_client_id_req(entry.data(), entry.size()));
}

TEST(TransferEntryReq, ResolveMethodIdx)
{
    const auto entry = create_entry();

    ASSERT_EQ(method_idx, get_method_idx_req(entry.data(), entry.size()));
}

TEST(TransferEntryReq, ResolveSerializedMessage)
{
    const auto entry = create_entry();

    ASSERT_EQ(serialized_message, get_serialized_message_req(entry.data(), entry.size()));
}
