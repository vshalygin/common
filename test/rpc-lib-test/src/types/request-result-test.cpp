#include <rpc-lib/types/request-result.h>

#include <gtest/gtest.h>

using namespace vshalygin::rpc;
using namespace testing;

TEST(RequestResult, TestIsSuccess)
{
    EXPECT_TRUE(is_success(request_result::ok));
    EXPECT_FALSE(is_success(request_result::timeout));
    EXPECT_FALSE(is_success(request_result::failed));
    EXPECT_FALSE(is_success(request_result::send_timeout));
    EXPECT_FALSE(is_success(request_result::send_failed));
    EXPECT_FALSE(is_success(request_result::send_canceled));
    EXPECT_FALSE(is_success(request_result::canceled));
    EXPECT_FALSE(is_success(request_result::request_not_processed));
    EXPECT_FALSE(is_success(request_result::request_too_big));
    EXPECT_FALSE(is_success(request_result::response_parse_error));
    EXPECT_FALSE(is_success(request_result::invalid_response));
    EXPECT_FALSE(is_success(request_result::no_connection));
    EXPECT_FALSE(is_success(request_result::unknown_error));
}

TEST(RequestResult, TestIsFail)
{
    EXPECT_FALSE(is_fail(request_result::ok));
    EXPECT_TRUE(is_fail(request_result::timeout));
    EXPECT_TRUE(is_fail(request_result::failed));
    EXPECT_TRUE(is_fail(request_result::send_timeout));
    EXPECT_TRUE(is_fail(request_result::send_failed));
    EXPECT_TRUE(is_fail(request_result::send_canceled));
    EXPECT_TRUE(is_fail(request_result::canceled));
    EXPECT_TRUE(is_fail(request_result::request_not_processed));
    EXPECT_TRUE(is_fail(request_result::request_too_big));
    EXPECT_TRUE(is_fail(request_result::response_parse_error));
    EXPECT_TRUE(is_fail(request_result::invalid_response));
    EXPECT_TRUE(is_fail(request_result::no_connection));
    EXPECT_TRUE(is_fail(request_result::unknown_error));
}

TEST(RequestResult, TestStringConversation)
{
    EXPECT_EQ(to_string(request_result::ok), "ok");
    EXPECT_EQ(to_string(request_result::timeout), "timeout");
    EXPECT_EQ(to_string(request_result::failed), "failed");
    EXPECT_EQ(to_string(request_result::send_timeout), "send_timeout");
    EXPECT_EQ(to_string(request_result::send_failed), "send_failed");
    EXPECT_EQ(to_string(request_result::send_canceled), "send_canceled");
    EXPECT_EQ(to_string(request_result::canceled), "canceled");
    EXPECT_EQ(to_string(request_result::request_not_processed), "request_not_processed");
    EXPECT_EQ(to_string(request_result::request_too_big), "request_too_big");
    EXPECT_EQ(to_string(request_result::response_parse_error), "response_parse_error");
    EXPECT_EQ(to_string(request_result::invalid_response), "invalid_response");
    EXPECT_EQ(to_string(request_result::no_connection), "no_connection");
    EXPECT_EQ(to_string(request_result::unknown_error), "unknown_error");
}
