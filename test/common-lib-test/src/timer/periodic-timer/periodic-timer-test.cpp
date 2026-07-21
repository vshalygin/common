#include <common-lib/timer/periodic-timer.h>
#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/synchronization/event.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <latch>

using namespace vshalygin::cl;
using namespace testing;

using callack_ret = periodic_timer::callback_ret;

class PeriodicTimer
    : public Test
{
protected:
    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(4);
    }

    void TearDown() override
    {
        m_thread_pool->stop();
    }

protected:
    std::shared_ptr<thread_pool> m_thread_pool;
};

TEST_F(PeriodicTimer, DoesNothingOnAttemptToCancelNotActiveTimer)
{
    periodic_timer sut(m_thread_pool->get_io_context());

    sut.stop();
}

TEST_F(PeriodicTimer, CallsCallbackInSpecifiedTimes)
{
    periodic_timer sut(m_thread_pool->get_io_context());
    MockFunction<callack_ret()> callback;
    EXPECT_CALL(callback, Call)
        .Times(2);

    sut.start(callback.AsStdFunction(), std::chrono::milliseconds(1), 2);

    while(sut.is_active()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(PeriodicTimer, ThrowsExceptionOnAttemptToStartTimerTwiceWhenFirstInNotCompleted)
{
    event sync_event;
    periodic_timer sut(m_thread_pool->get_io_context());

    MockFunction<callack_ret()> callback;
    sut.start(callback.AsStdFunction(), std::chrono::milliseconds(1));
    ASSERT_ANY_THROW(sut.start(callback.AsStdFunction(), std::chrono::milliseconds(1)));
}

TEST_F(PeriodicTimer, AnwersNotActiveJustAfterCreation)
{
    periodic_timer sut(m_thread_pool->get_io_context());

    const auto ans = sut.is_active();

    ASSERT_FALSE(ans);
}

TEST_F(PeriodicTimer, AnwersActiveAfterStart)
{
    periodic_timer sut(m_thread_pool->get_io_context());

    sut.start([]() { return callack_ret::Continue; }, std::chrono::milliseconds(1));
    const auto ans = sut.is_active();

    ASSERT_TRUE(ans);
}

TEST_F(PeriodicTimer, StartsTimerAfterPreviousWasCanceled)
{
    periodic_timer sut(m_thread_pool->get_io_context());

    sut.start([]() { return callack_ret::Continue; }, std::chrono::seconds(10), 2);
    sut.stop();

    MockFunction<callack_ret()> callback;
    EXPECT_CALL(callback, Call)
        .Times(2);

    ASSERT_NO_THROW(sut.start(callback.AsStdFunction(), std::chrono::milliseconds(1), 2));

    while(sut.is_active());

    Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(PeriodicTimer, StartsTimerAfterPreviousWasCompletedPeriods)
{
    periodic_timer sut(m_thread_pool->get_io_context());
    sut.start([]() { return callack_ret::Continue; }, std::chrono::milliseconds(1), 2);
    while(sut.is_active()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    MockFunction<callack_ret()> callback;
    EXPECT_CALL(callback, Call)
        .Times(2);

    ASSERT_NO_THROW(sut.start(callback.AsStdFunction(), std::chrono::milliseconds(1), 2));

    while(sut.is_active());
    Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(PeriodicTimer, CatchesExceptionInCallback)
{
    periodic_timer sut(m_thread_pool->get_io_context());

    MockFunction<callack_ret()> callback;
    EXPECT_CALL(callback, Call)
        .Times(2);

    sut.start(callback.AsStdFunction(), std::chrono::milliseconds(1), 2);

    while(sut.is_active()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(PeriodicTimer, CancelsTimerOnDestruction)
{
    auto start = std::chrono::steady_clock::now();

    {
        periodic_timer sut(m_thread_pool->get_io_context());
        sut.start([]() { return callack_ret::Continue; }, std::chrono::seconds(20));
    }

    ASSERT_TRUE(std::chrono::steady_clock::now() - start < std::chrono::seconds(10));
}

TEST_F(PeriodicTimer, CancelsTimerInCallback)
{
    periodic_timer sut(m_thread_pool->get_io_context());

    std::atomic_int counter = 0;
    sut.start([&]() { ++counter; return callack_ret::Abort; }, std::chrono::milliseconds(1));

    while(counter == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    ASSERT_EQ(counter, 1);
    ASSERT_FALSE(sut.is_active());
}

TEST_F(PeriodicTimer, AllowsToCancelInTwoThreadSimultaneously)
{
    periodic_timer sut(m_thread_pool->get_io_context());
    event sync_event1;
    event sync_event2;
    std::latch sync_latch(2);
    MockFunction<callack_ret()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event1.set();  sync_event2.wait(); return callack_ret::Continue; });

    sut.start(callback.AsStdFunction(), std::chrono::milliseconds(1));
    sync_event1.wait();

    m_thread_pool->post([&]() { sync_latch.count_down(); sut.stop(); });
    m_thread_pool->post([&]() { sync_latch.count_down();  sut.stop(); });
    sync_latch.wait();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sync_event2.set();

    m_thread_pool->stop();
}

TEST_F(PeriodicTimer, IsNotActiveAfterCancelCall)
{
    periodic_timer sut(m_thread_pool->get_io_context());

    sut.start([]() { return callack_ret::Continue; }, std::chrono::minutes(10));
    sut.stop();

    ASSERT_FALSE(sut.is_active());
}
