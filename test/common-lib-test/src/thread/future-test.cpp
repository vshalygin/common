#include <common-lib/thread/future.h>
#include <common-lib/thread/thread-pool.h>

#include <gtest/gtest.h>
#include <type_traits>

using namespace vshalygin::cl;
using namespace testing;

static_assert(!std::is_copy_constructible_v<future<int>>);
static_assert(!std::is_copy_assignable_v<future<int>>);
static_assert(std::is_move_constructible_v<future<int>>);
static_assert(std::is_move_assignable_v<future<int>>);

static_assert(!std::is_copy_constructible_v<promise<int>>);
static_assert(!std::is_copy_assignable_v<promise<int>>);
static_assert(std::is_move_constructible_v<promise<int>>);
static_assert(std::is_move_assignable_v<promise<int>>);

class Future
    : public Test
{
protected:

protected:
    thread_pool m_pool{ 2 };
};

TEST_F(Future, Init)
{
    promise<int> promise(&m_pool, []() -> int { return 1; });
    auto future = promise.resolve();
    ASSERT_EQ(future.get(), 1);
}

TEST_F(Future, ExecutesSuccessCallback)
{
    int i = 0;
    promise<int> promise(&m_pool, []() { return 2; });
    auto future = promise.resolve();
    future.then([&i](int &&ii) { i = ii; return 0; })
          .get();

    ASSERT_EQ(i, 2);
}

TEST_F(Future, PromiseIsValidAfterCreation)
{
    promise<int> sut(&m_pool, []() { return 1; });

    ASSERT_TRUE(sut.is_valid());
}

TEST_F(Future, PromiseIsNotValidAfterMove)
{
    promise<int> sut(&m_pool, []() { return 1; });
    promise<int> other(std::move(sut));

    EXPECT_TRUE(other.is_valid());
    EXPECT_FALSE(sut.is_valid());
}

TEST_F(Future, PromiseIsNotValidAfterMoveAssignment)
{
    promise<int> sut(&m_pool, []() { return 1; });
    promise<int> other(&m_pool, []() { return 1; });

    other = std::move(sut);

    EXPECT_TRUE(other.is_valid());
    ASSERT_FALSE(sut.is_valid());
}

TEST_F(Future, DefaultCreatedPromiseIsNotValid)
{
    promise<int> sut;

    ASSERT_FALSE(sut.is_valid());
}

TEST_F(Future, DefaultCreatedPromiseIsValidAfterAssigningValidPromise)
{
    promise<int> sut;
    sut = promise<int>(&m_pool, []() { return 0; });

    ASSERT_TRUE(sut.is_valid());
}

TEST_F(Future, DefaultCreatedFutureIsNotValid)
{
    future<int> sut;

    ASSERT_FALSE(sut.is_valid());
}

TEST_F(Future, FutureCreatedFromPromiseIsValid)
{
    auto p = promise<int>(&m_pool, []() { return 2; });
    auto future = p.resolve();

    ASSERT_TRUE(future.is_valid());
    ASSERT_EQ(future.get(), 2);
}

TEST_F(Future, FutureIsInvalidAfterMove)
{
    auto p = promise<int>(&m_pool, []() { return 2; });
    auto future = p.resolve();
    auto other_future(std::move(future));

    ASSERT_FALSE(future.is_valid());
    ASSERT_TRUE(other_future.is_valid());
    ASSERT_EQ(other_future.get(), 2);
}

TEST_F(Future, FutureIsInvalidAfterMoveAssign)
{
    auto p1 = promise<int>(&m_pool, []() { return 1; });
    auto p2 = promise<int>(&m_pool, []() { return 2; });
    auto future1 = p1.resolve();
    auto future2 = p2.resolve();
    future2 = std::move(future1);

    ASSERT_FALSE(future1.is_valid());
    ASSERT_TRUE(future2.is_valid());
    ASSERT_EQ(future2.get(), 1);
}

TEST_F(Future, FutureIsValidAfterCorrespondingPromiseDestoyed)
{
    auto future = promise<int>(&m_pool, []() { return 2; }).resolve();

    ASSERT_TRUE(future.is_valid());
    ASSERT_EQ(future.get(), 2);
}
