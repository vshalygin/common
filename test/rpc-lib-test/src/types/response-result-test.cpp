#include <rpc-lib/types/response-result.h>

#include <gtest/gtest.h>

using namespace vsh::rpc;
using namespace testing;

TEST(ResponseResult, TestIsSucceess)
{
    EXPECT_TRUE(is_success(response_result::ok));
    EXPECT_FALSE(is_success(response_result::canceled));
    EXPECT_FALSE(is_success(response_result::insufficient_rights));
    EXPECT_FALSE(is_success(response_result::request_parse_error));
    EXPECT_FALSE(is_success(response_result::response_too_big));
    EXPECT_FALSE(is_success(response_result::not_implemented));
    EXPECT_FALSE(is_success(response_result::unknown_error));
}

TEST(ResponseResult, TestIsFail)
{
    EXPECT_FALSE(is_fail(response_result::ok));
    EXPECT_TRUE(is_fail(response_result::canceled));
    EXPECT_TRUE(is_fail(response_result::insufficient_rights));
    EXPECT_TRUE(is_fail(response_result::request_parse_error));
    EXPECT_TRUE(is_fail(response_result::response_too_big));
    EXPECT_TRUE(is_fail(response_result::not_implemented));
    EXPECT_TRUE(is_fail(response_result::unknown_error));
}

TEST(ResponseResult, TestStringConversation)
{
    EXPECT_EQ(response_result::ok,
              response_result_from_string(to_string(response_result::ok)));
    EXPECT_EQ(response_result::canceled,
              response_result_from_string(to_string(response_result::canceled)));
    EXPECT_EQ(response_result::insufficient_rights,
              response_result_from_string(to_string(response_result::insufficient_rights)));
    EXPECT_EQ(response_result::request_parse_error,
              response_result_from_string(to_string(response_result::request_parse_error)));
    EXPECT_EQ(response_result::response_too_big,
              response_result_from_string(to_string(response_result::response_too_big)));
    EXPECT_EQ(response_result::not_implemented,
              response_result_from_string(to_string(response_result::not_implemented)));
    EXPECT_EQ(response_result::unknown_error,
              response_result_from_string(to_string(response_result::unknown_error)));
}
