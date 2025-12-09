#include <common-lib/timer/periodic-timer/periodic-timer.h>
#include <common-lib/thread-pool/thread-pool.h>
#include <common-lib/syncronization/event/event.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vsh::cl;
using namespace testing;

TEST(PeriodicTimer, DoesNothingOnAttemptToCancelNotActiveTimer)
{
    thread_pool pool(2);
    periodic_timer sut(pool.get_io_context());

    sut.cancel();
}

TEST(PeriodicTimer, CallsCallbackInSpecifiedTimes)
{
    thread_pool pool(2);
    periodic_timer sut(pool.get_io_context());
    sut.set_periods_count(2);
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(2);

    sut.start(callback.AsStdFunction(), std::chrono::milliseconds(1));

    while(sut.is_active()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    Mock::VerifyAndClearExpectations(&callback);
}

TEST(PeriodicTimer, ThrowsExceptionOnAttemptToStartTimerTwiceWhenFirstInNotCompleted)
{
    event sync_event;
    thread_pool pool(2);
    periodic_timer sut(pool.get_io_context());

    MockFunction<void()> callback;
    sut.start(callback.AsStdFunction(), std::chrono::milliseconds(1));
    ASSERT_ANY_THROW(sut.start(callback.AsStdFunction(), std::chrono::milliseconds(1)));
}

TEST(PeriodicTimer, AnwersNotActiveJustAfterCreation)
{
    thread_pool pool(2);
    periodic_timer sut(pool.get_io_context());

    const auto ans = sut.is_active();

    ASSERT_FALSE(ans);
}

TEST(PeriodicTimer, AnwersActiveAfterStart)
{
    thread_pool pool(2);
    periodic_timer sut(pool.get_io_context());

    sut.start([]() {}, std::chrono::milliseconds(1));
    const auto ans = sut.is_active();

    ASSERT_TRUE(ans);
}

TEST(PeriodicTimer, StartsTimerAfterPreviousWasCanceled)
{
    thread_pool pool(2);
    periodic_timer sut(pool.get_io_context());
    sut.set_periods_count(2);

    sut.start([]() {}, std::chrono::seconds(10));
    sut.cancel();

    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(2);

    ASSERT_NO_THROW(sut.start(callback.AsStdFunction(), std::chrono::milliseconds(1)));

    while(sut.is_active()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    Mock::VerifyAndClearExpectations(&callback);
}

TEST(PeriodicTimer, StartsTimerAfterPreviousWasCompletedPeriods)
{
    thread_pool pool(2);
    periodic_timer sut(pool.get_io_context());
    sut.set_periods_count(2);
    sut.start([]() {}, std::chrono::milliseconds(1));
    while(sut.is_active()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(2);

    ASSERT_NO_THROW(sut.start(callback.AsStdFunction(), std::chrono::milliseconds(1)));

    while(sut.is_active()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    Mock::VerifyAndClearExpectations(&callback);
}

TEST(PeriodicTimer, CatchesExceptionInCallback)
{
    thread_pool pool(2);
    periodic_timer sut(pool.get_io_context());
    sut.set_periods_count(2);

    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(2);

    sut.start(callback.AsStdFunction(), std::chrono::milliseconds(1));

    while(sut.is_active()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    Mock::VerifyAndClearExpectations(&callback);
}

TEST(PeriodicTimer, ClearsPeriodsCount)
{
    thread_pool pool(2);
    periodic_timer sut(pool.get_io_context());
    sut.set_periods_count(2);
    sut.clear_periods_count();

    std::atomic_int counter = 0;
    sut.start([&]() { ++counter; }, std::chrono::milliseconds(1));

    while(counter < 3) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

TEST(PeriodicTimer, CancelsTimerOnDestruction)
{
    auto start = std::chrono::steady_clock::now();

    {
        thread_pool pool(2);
        periodic_timer sut(pool.get_io_context());
        sut.start([]() {}, std::chrono::seconds(20));
    }

    ASSERT_TRUE(std::chrono::steady_clock::now() - start < std::chrono::seconds(10));
}
