#include <rpc-lib/types/response-result.h>

#include <gtest/gtest.h>

using namespace vshalygin::rpc;
using namespace testing;

TEST(ResponseResult, TestIsSucceess)
{
    EXPECT_TRUE(is_success(response_result::ok));
    EXPECT_FALSE(is_success(response_result::canceled));
    EXPECT_FALSE(is_success(response_result::request_parse_error));
    EXPECT_FALSE(is_success(response_result::response_too_big));
    EXPECT_FALSE(is_success(response_result::not_implemented));
    EXPECT_FALSE(is_success(response_result::invalid_request));
    EXPECT_FALSE(is_success(response_result::unknown_error));
}

TEST(ResponseResult, TestIsFail)
{
    EXPECT_FALSE(is_fail(response_result::ok));
    EXPECT_TRUE(is_fail(response_result::canceled));
    EXPECT_TRUE(is_fail(response_result::request_parse_error));
    EXPECT_TRUE(is_fail(response_result::response_too_big));
    EXPECT_TRUE(is_fail(response_result::not_implemented));
    EXPECT_TRUE(is_fail(response_result::invalid_request));
    EXPECT_TRUE(is_fail(response_result::unknown_error));
}

TEST(ResponseResult, TestStringConversation)
{
    EXPECT_EQ(to_string(response_result::ok), "ok");
    EXPECT_EQ(to_string(response_result::canceled), "canceled");
    EXPECT_EQ(to_string(response_result::request_parse_error), "request_parse_error");
    EXPECT_EQ(to_string(response_result::response_too_big), "response_too_big");
    EXPECT_EQ(to_string(response_result::not_implemented), "not_implemented");
    EXPECT_EQ(to_string(response_result::invalid_request), "invalid_request");
    EXPECT_EQ(to_string(response_result::unknown_error), "unknown_error");
}
