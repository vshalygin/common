#include <rpc-lib/types/request-result.h>

#include <gtest/gtest.h>

using namespace vshalygin::rpc;
using namespace testing;

TEST(RequestResult, TestIsSuccess)
{
    EXPECT_TRUE(is_success(request_result::ok));
    EXPECT_FALSE(is_success(request_result::timeout));
    EXPECT_FALSE(is_success(request_result::send_timeout_error));
    EXPECT_FALSE(is_success(request_result::send_unknown_error));
    EXPECT_FALSE(is_success(request_result::send_canceled_error));
    EXPECT_FALSE(is_success(request_result::canceled));
    EXPECT_FALSE(is_success(request_result::request_not_processed));
    EXPECT_FALSE(is_success(request_result::response_parse_error));
    EXPECT_FALSE(is_success(request_result::no_connection));
    EXPECT_FALSE(is_success(request_result::unknown_error));
}

TEST(RequestResult, TestIsFail)
{
    EXPECT_FALSE(is_fail(request_result::ok));
    EXPECT_TRUE(is_fail(request_result::timeout));
    EXPECT_TRUE(is_fail(request_result::send_timeout_error));
    EXPECT_TRUE(is_fail(request_result::send_unknown_error));
    EXPECT_TRUE(is_fail(request_result::send_canceled_error));
    EXPECT_TRUE(is_fail(request_result::canceled));
    EXPECT_TRUE(is_fail(request_result::request_not_processed));
    EXPECT_TRUE(is_fail(request_result::response_parse_error));
    EXPECT_TRUE(is_fail(request_result::no_connection));
    EXPECT_TRUE(is_fail(request_result::unknown_error));
}

TEST(RequestResult, TestStringConversation)
{
    EXPECT_EQ(to_string(request_result::ok), "ok");
    EXPECT_EQ(to_string(request_result::timeout), "timeout");
    EXPECT_EQ(to_string(request_result::send_timeout_error), "send_timeout_error");
    EXPECT_EQ(to_string(request_result::send_unknown_error), "send_unknown_error");
    EXPECT_EQ(to_string(request_result::send_canceled_error), "send_canceled_error");
    EXPECT_EQ(to_string(request_result::canceled), "canceled");
    EXPECT_EQ(to_string(request_result::request_not_processed), "request_not_processed");
    EXPECT_EQ(to_string(request_result::response_parse_error), "response_parse_error");
    EXPECT_EQ(to_string(request_result::no_connection), "no_connection");
    EXPECT_EQ(to_string(request_result::unknown_error), "unknown_error");
}
