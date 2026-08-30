#include <common-lib/thread/future/future.h>
#include <common-lib/thread/thread-pool/thread-pool.h>

#include <gtest/gtest.h>

#include <atomic>
#include <exception>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>

using namespace vshalygin::cl;
using namespace testing;

static_assert(!std::is_copy_constructible_v<promise<thread_pool, int>>);
static_assert(!std::is_copy_assignable_v<promise<thread_pool, int>>);
static_assert(std::is_move_constructible_v<promise<thread_pool, int>>);
static_assert(std::is_move_assignable_v<promise<thread_pool, int>>);

namespace {
    void expect_future_error(const std::future_error &error,
                             std::future_errc expected)
    {
        EXPECT_EQ(error.code(), std::make_error_code(expected));
    }
}

class Promise
    : public Test
{
protected:
    void TearDown() override
    {
        m_thread_pool.stop();
    }

    thread_pool m_thread_pool{ 2 };
};

TEST_F(Promise, IsValidAfterConstruction)
{
    promise<thread_pool, int> sut(&m_thread_pool);
    EXPECT_TRUE(sut.is_valid());
}

TEST_F(Promise, DefaultConstructedPromiseIsInvalid)
{
    promise<thread_pool, int> sut;
    EXPECT_FALSE(sut.is_valid());
    EXPECT_THROW(sut.get_future(), std::logic_error);
    EXPECT_THROW(sut.set_value(1), std::logic_error);
}

TEST_F(Promise, FutureMayBeRetrievedOnlyOnce)
{
    promise<thread_pool, int> sut(&m_thread_pool);
    auto result = sut.get_future();
    EXPECT_TRUE(result.is_valid());
    EXPECT_THROW(sut.get_future(), std::logic_error);
}

TEST_F(Promise, SetValueCompletesValueFuture)
{
    promise<thread_pool, int> sut(&m_thread_pool);
    auto result = sut.get_future();
    sut.set_value(42);
    EXPECT_EQ(*result.get().lock(), 42);
}

TEST_F(Promise, SetValueCompletesVoidFuture)
{
    promise<thread_pool, void> sut(&m_thread_pool);
    auto result = sut.get_future();
    sut.set_value();
    EXPECT_NO_THROW(result.get());
}

TEST_F(Promise, SetValueMovesMoveOnlyValue)
{
    promise<thread_pool, std::unique_ptr<int>> sut(&m_thread_pool);
    auto result = sut.get_future();
    auto source = std::make_unique<int>(42);
    sut.set_value(std::move(source));
    EXPECT_FALSE(source);
    auto value = result.get().lock();
    ASSERT_TRUE(*value);
    EXPECT_EQ(**value, 42);
}

TEST_F(Promise, SetValuePreservesReference)
{
    int source = 42;
    promise<thread_pool, int &> sut(&m_thread_pool);
    auto result = sut.get_future();
    sut.set_value(source);
    auto value = result.get().lock();
    EXPECT_EQ(&*value, &source);
}

TEST_F(Promise, SetValueStoresFutureTuple)
{
    promise<thread_pool, ftuple<int, std::unique_ptr<int>>> sut(&m_thread_pool);
    auto result = sut.get_future();

    sut.set_value(ftuple(19, std::make_unique<int>(23)));

    auto value = result.get().lock();
    value.with([](int left, const std::unique_ptr<int> &right) {
        ASSERT_TRUE(right);
        EXPECT_EQ(left + *right, 42);
    });
}

TEST_F(Promise, SetExceptionPropagatesException)
{
    promise<thread_pool, int> sut(&m_thread_pool);
    auto result = sut.get_future();
    sut.set_exception(std::make_exception_ptr(std::runtime_error("failure")));

    try {
        (void)result.get();
        FAIL();
    } catch(const std::runtime_error &error) {
        EXPECT_STREQ(error.what(), "failure");
    }
}

TEST_F(Promise, NullExceptionDoesNotCompletePromise)
{
    promise<thread_pool, int> sut(&m_thread_pool);
    auto result = sut.get_future();
    EXPECT_THROW(sut.set_exception({}), std::invalid_argument);
    sut.set_value(42);
    EXPECT_EQ(*result.get().lock(), 42);
}

TEST_F(Promise, RepeatedCompletionThrowsPromiseAlreadySatisfied)
{
    promise<thread_pool, int> sut(&m_thread_pool);
    sut.set_value(1);

    try {
        sut.set_exception(std::make_exception_ptr(std::runtime_error("ignored")));
        FAIL();
    } catch(const std::future_error &error) {
        expect_future_error(error, std::future_errc::promise_already_satisfied);
    }

    try {
        sut.set_value(2);
        FAIL();
    } catch(const std::future_error &error) {
        expect_future_error(error, std::future_errc::promise_already_satisfied);
    }
}

TEST_F(Promise, SetExceptionPreventsLaterSetValue)
{
    promise<thread_pool, int> sut(&m_thread_pool);
    sut.set_exception(std::make_exception_ptr(std::runtime_error("failure")));

    try {
        sut.set_value(42);
        FAIL();
    } catch(const std::future_error &error) {
        expect_future_error(error, std::future_errc::promise_already_satisfied);
    }
}

TEST_F(Promise, MoveConstructionTransfersCompletionResponsibility)
{
    promise<thread_pool, int> source(&m_thread_pool);
    auto result = source.get_future();
    promise<thread_pool, int> destination(std::move(source));
    destination.set_value(42);
    EXPECT_FALSE(source.is_valid());
    EXPECT_EQ(*result.get().lock(), 42);
}

TEST_F(Promise, MoveAssignmentBreaksReplacedPromise)
{
    promise<thread_pool, int> destination(&m_thread_pool);
    auto replaced_result = destination.get_future();
    promise<thread_pool, int> source(&m_thread_pool);
    auto source_result = source.get_future();

    destination = std::move(source);
    destination.set_value(42);

    try {
        (void)replaced_result.get();
        FAIL();
    } catch(const std::future_error &error) {
        expect_future_error(error, std::future_errc::broken_promise);
    }
    EXPECT_EQ(*source_result.get().lock(), 42);
}

TEST_F(Promise, DestructionOfUncompletedPromiseReportsBrokenPromise)
{
    future<thread_pool, int> result;
    {
        promise<thread_pool, int> sut(&m_thread_pool);
        result = sut.get_future();
    }

    try {
        (void)result.get();
        FAIL();
    } catch(const std::future_error &error) {
        expect_future_error(error, std::future_errc::broken_promise);
    }
}

TEST_F(Promise, ConcurrentCompletionHasExactlyOneWinner)
{
    promise<thread_pool, int> sut(&m_thread_pool);
    auto result = sut.get_future();
    std::atomic<unsigned> successes{ 0 };
    std::atomic<unsigned> already_satisfied{ 0 };
    std::atomic<unsigned> unexpected_errors{ 0 };

    auto complete = [&sut,
                     &successes,
                     &already_satisfied,
                     &unexpected_errors](int value) {
        try {
            sut.set_value(value);
            ++successes;
        } catch(const std::future_error &error) {
            if(error.code() == std::make_error_code(
                                   std::future_errc::promise_already_satisfied)) {
                ++already_satisfied;
            } else {
                ++unexpected_errors;
            }
        } catch(...) {
            ++unexpected_errors;
        }
    };

    std::thread first(complete, 1);
    std::thread second(complete, 2);
    first.join();
    second.join();

    EXPECT_EQ(successes, 1U);
    EXPECT_EQ(already_satisfied, 1U);
    EXPECT_EQ(unexpected_errors, 0U);
    const auto value = *result.get().lock();
    EXPECT_TRUE(value == 1 || value == 2);
}

TEST_F(Promise, ValueAndExceptionShareOneCompletionRight)
{
    promise<thread_pool, int> sut(&m_thread_pool);
    auto result = sut.get_future();
    std::atomic<unsigned> successes{ 0 };
    std::atomic<unsigned> already_satisfied{ 0 };

    auto run = [&successes, &already_satisfied](auto &&completion) {
        try {
            completion();
            ++successes;
        } catch(const std::future_error &error) {
            if(error.code() == std::make_error_code(
                                   std::future_errc::promise_already_satisfied)) {
                ++already_satisfied;
            }
        }
    };

    std::thread value_setter([&] { run([&] { sut.set_value(1); }); });
    std::thread exception_setter([&] {
        run([&] {
            sut.set_exception(
                std::make_exception_ptr(std::runtime_error("failure")));
        });
    });
    value_setter.join();
    exception_setter.join();

    EXPECT_EQ(successes, 1U);
    EXPECT_EQ(already_satisfied, 1U);
    result.wait();
    EXPECT_NE(result.has_value(), result.has_exception());
}
