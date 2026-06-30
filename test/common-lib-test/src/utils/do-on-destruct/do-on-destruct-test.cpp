#include <common-lib/utils/do-on-destruct.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

using namespace vshalygin::cl;
using namespace testing;

TEST(DoOnDestruct, BaseTest)
{
    MockFunction<void()> mock;
    EXPECT_CALL(mock, Call)
        .Times(1);

    do_on_destruct d(mock.AsStdFunction());
}

TEST(DoOnDestruct, ReleasesFunction)
{
    MockFunction<void()> mock;
    EXPECT_CALL(mock, Call)
        .Times(0);

    do_on_destruct d(mock.AsStdFunction());
    d.release();
}

TEST(DoOnDestruct, NotCrushIfFunctionThrows)
{
    MockFunction<void()> mock;
    EXPECT_CALL(mock, Call)
        .Times(1)
        .WillOnce(Throw(std::runtime_error("")));

    do_on_destruct d(mock.AsStdFunction());
}

TEST(DoOnDestruct, CopiesFunctionIntoInnerHolder)
{
    bool is_called = false;
    auto func = std::make_unique<std::function<void()>>([&]() { is_called = true; });

    {
        do_on_destruct<decltype(*func)> d(*func);
        func.reset();
    }
    
    ASSERT_TRUE(is_called);
}

TEST(DoOnDestruct, MovesFunctionIntoInnerHolder)
{
    bool is_called = false;
    auto m = std::make_unique<int>(2);
    auto func = [&is_called, m = std::move(m)]() {
        EXPECT_EQ(*m, 2);
        is_called = true;
    };

    {
        do_on_destruct d(std::move(func));
    }

    ASSERT_TRUE(is_called);
}
