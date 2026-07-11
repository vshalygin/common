#include <common-lib/timer/periodic-timer/periodic-timer.h>
#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/synchronization/event/event.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <latch>

using namespace vshalygin::cl;
using namespace testing;

using callack_ret = periodic_timer::callback_ret;

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
    MockFunction<callack_ret()> callback;
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

    MockFunction<callack_ret()> callback;
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

    sut.start([]() { return callack_ret::Continue; }, std::chrono::milliseconds(1));
    const auto ans = sut.is_active();

    ASSERT_TRUE(ans);
}

TEST(PeriodicTimer, StartsTimerAfterPreviousWasCanceled)
{
    thread_pool pool(2);
    periodic_timer sut(pool.get_io_context());
    sut.set_periods_count(2);

    sut.start([]() { return callack_ret::Continue; }, std::chrono::seconds(10));
    sut.cancel();

    MockFunction<callack_ret()> callback;
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
    sut.start([]() { return callack_ret::Continue; }, std::chrono::milliseconds(1));
    while(sut.is_active()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    MockFunction<callack_ret()> callback;
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

    MockFunction<callack_ret()> callback;
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
    sut.start([&]() { ++counter; return callack_ret::Continue; }, std::chrono::milliseconds(1));

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
        sut.start([]() { return callack_ret::Continue; }, std::chrono::seconds(20));
    }

    ASSERT_TRUE(std::chrono::steady_clock::now() - start < std::chrono::seconds(10));
}

TEST(PeriodicTimer, CancelsTimerInCallback)
{
    thread_pool pool(2);
    periodic_timer sut(pool.get_io_context());
    sut.set_periods_count(2);
    sut.clear_periods_count();

    std::atomic_int counter = 0;
    sut.start([&]() { ++counter; return callack_ret::Abort; }, std::chrono::milliseconds(1));

    while(counter == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    ASSERT_EQ(counter, 1);
    ASSERT_FALSE(sut.is_active());
}

TEST(PeriodicTimer, AllowsToCancelInTwoThreadSimultaneously)
{
    thread_pool pool(4);
    periodic_timer sut(pool.get_io_context());
    event sync_event1;
    event sync_event2;
    std::latch sync_latch(2);
    MockFunction<callack_ret()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event1.set();  sync_event2.wait(); return callack_ret::Continue; });

    sut.start(callback.AsStdFunction(), std::chrono::milliseconds(1));
    sync_event1.wait();

    pool.post([&]() { sync_latch.count_down(); sut.cancel(); });
    pool.post([&]() { sync_latch.count_down();  sut.cancel(); });
    sync_latch.wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sync_event2.set();

    pool.stop();
}

TEST(PeriodicTimer, IsNotActiveAfterCancelCall)
{
    thread_pool pool(4);
    periodic_timer sut(pool.get_io_context());

    sut.start([]() { return callack_ret::Continue; }, std::chrono::minutes(10));
    sut.cancel();

    ASSERT_FALSE(sut.is_active());
}
