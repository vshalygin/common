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
    EXPECT_TRUE(is_fail(request_result::unknown_error));
}

TEST(RequestResult, TestStringConversation)
{
    EXPECT_EQ(request_result::ok,
              request_result_from_string(to_string(request_result::ok)));
    EXPECT_EQ(request_result::timeout,
              request_result_from_string(to_string(request_result::timeout)));
    EXPECT_EQ(request_result::send_timeout_error,
              request_result_from_string(to_string(request_result::send_timeout_error)));
    EXPECT_EQ(request_result::send_unknown_error,
              request_result_from_string(to_string(request_result::send_unknown_error)));
    EXPECT_EQ(request_result::send_canceled_error,
              request_result_from_string(to_string(request_result::send_canceled_error)));
    EXPECT_EQ(request_result::canceled,
              request_result_from_string(to_string(request_result::canceled)));
    EXPECT_EQ(request_result::request_not_processed,
              request_result_from_string(to_string(request_result::request_not_processed)));
    EXPECT_EQ(request_result::response_parse_error,
              request_result_from_string(to_string(request_result::response_parse_error)));
    EXPECT_EQ(request_result::unknown_error,
              request_result_from_string(to_string(request_result::unknown_error)));
}
