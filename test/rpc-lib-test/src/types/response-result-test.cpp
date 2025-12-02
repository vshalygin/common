#include <rpc-lib/types/response-result.h>

#include <gtest/gtest.h>

using namespace vsh::rpc;
using namespace testing;

TEST(ResponseResult, TestIsSucceess)
{
    EXPECT_TRUE(is_success(response_result::ok));
    EXPECT_FALSE(is_success(response_result::unknown_error));
}

TEST(ResponseResult, TestIsFail)
{
    EXPECT_FALSE(is_fail(response_result::ok));
    EXPECT_TRUE(is_fail(response_result::unknown_error));
}
