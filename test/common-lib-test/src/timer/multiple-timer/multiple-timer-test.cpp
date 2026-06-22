#include <common-lib/timer/multiple-timer/multiple-timer.h>
#include <common-lib/thread-pool/thread-pool.h>
#include <common-lib/synchronization/event/event.h>
#include <common-lib/synchronization/latch/latch.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::cl;
using namespace testing;

class MultipleTimer
    : public Test
{
protected:
    void SetUp() override
    {
        m_thread_pool = std::make_unique<thread_pool>(4);
        m_multiple_timer = std::make_unique<multiple_timer>(m_thread_pool->get_io_context());
    }

protected:
    std::unique_ptr<thread_pool> m_thread_pool;
    std::unique_ptr<multiple_timer> m_multiple_timer;
};

TEST_F(MultipleTimer, AnswersZeroActiveTimesAfterCreation)
{
    ASSERT_EQ(m_multiple_timer->get_active_timers_count(), 0);
}

TEST_F(MultipleTimer, ExecuteCallbackOnTimeout)
{
    event sync_event;
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    m_multiple_timer->start(callback.AsStdFunction(), std::chrono::microseconds(3));

    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(MultipleTimer, IsAbleToHandleTwoTimers)
{
    m_multiple_timer->start([]() {}, std::chrono::seconds(10));
    m_multiple_timer->start([]() {}, std::chrono::seconds(10));

    EXPECT_EQ(m_multiple_timer->get_active_timers_count(), 2);
}

TEST_F(MultipleTimer, CancelsTimeoutById)
{
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(0);
    const auto id = m_multiple_timer->start(callback.AsStdFunction(), std::chrono::seconds(10));

    m_multiple_timer->cancel(id);
}

TEST_F(MultipleTimer, CallsTimeoutIfTimerWasTriedToCancelByWrongId)
{
    std::atomic_bool canary = false;
    event sync_event;
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { EXPECT_TRUE(canary);  sync_event.set(); });
    const auto id = m_multiple_timer->start(callback.AsStdFunction(), std::chrono::milliseconds(10));

    m_multiple_timer->cancel(id+1);
    canary = true;
    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(MultipleTimer, CancelsAllTimeouts)
{
    MockFunction<void()> callback1;
    EXPECT_CALL(callback1, Call)
        .Times(0);
    MockFunction<void()> callback2;
    EXPECT_CALL(callback2, Call)
        .Times(0);
    m_multiple_timer->start(callback1.AsStdFunction(), std::chrono::seconds(10));
    m_multiple_timer->start(callback2.AsStdFunction(), std::chrono::seconds(10));

    m_multiple_timer->cancel_all();
}

TEST_F(MultipleTimer, AnswersZeroCurrentTimerAfterTimerExpired)
{
    event sync_event;
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    m_multiple_timer->start(callback.AsStdFunction(), std::chrono::microseconds(1));

    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(1)));
    ASSERT_EQ(m_multiple_timer->get_active_timers_count(), 0);
}

TEST_F(MultipleTimer, CallsCancelInCallbackWithoutDeadlock)
{
    event sync_event;
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { m_multiple_timer->cancel_all(), sync_event.set(); });
    m_multiple_timer->start(callback.AsStdFunction(), std::chrono::microseconds(1));

    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}