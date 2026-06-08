#include <common-lib/thread-pool/strand.h>
#include <common-lib/thread-pool/thread-pool.h>
#include <common-lib/syncronization/event/event.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::cl;
using namespace testing;

TEST(Strand, ExecuteTasksConsecutively)
{
    event sync_event;
    MockFunction<void()> mock1, mock2;
    InSequence seq;
    EXPECT_CALL(mock1, Call())
        .Times(1)
        .WillOnce([]() { std::this_thread::sleep_for(std::chrono::milliseconds(10)); });
    EXPECT_CALL(mock2, Call())
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    thread_pool pool(2);
    auto sut = pool.create_strand();
    sut.post(mock1.AsStdFunction());
    sut.post(mock2.AsStdFunction());

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST(Strand, ExecuteTaskAfterDestruction)
{
    event sync_event1;
    event sync_event2;
    MockFunction<void()> mock1, mock2;
    EXPECT_CALL(mock1, Call())
        .Times(1)
        .WillOnce([&]() { sync_event1.wait(); });
    EXPECT_CALL(mock2, Call())
        .Times(1)
        .WillOnce([&]() { sync_event2.set(); });
    thread_pool pool(2);

    {
        auto sut = pool.create_strand();
        sut.post(mock1.AsStdFunction());
        sut.post(mock2.AsStdFunction());
    }
    sync_event1.set();

    sync_event2.wait_for(std::chrono::seconds(10));
}


TEST(Strand, DispatchesTaskByPostingInExecutionQueue)
{
    event sync_event;
    thread_pool pool(2);
    auto sut = pool.create_strand();
    InSequence s;
    MockFunction<void()> mock1, mock2;
    EXPECT_CALL(mock1, Call())
        .Times(1);
    EXPECT_CALL(mock2, Call())
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    sut.dispatch(mock1.AsStdFunction());
    sut.dispatch(mock2.AsStdFunction());
    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST(Strand, DispatchesTaskByExecutingInPlace)
{
    event sync_event1, sync_event2;
    thread_pool pool(2);
    auto sut = pool.create_strand();
    InSequence s;
    MockFunction<void()> mock1, mock2, mock3;
    EXPECT_CALL(mock1, Call())
        .Times(1)
        .WillOnce([&]() {
            sut.dispatch(mock2.AsStdFunction());
        });
    EXPECT_CALL(mock2, Call())
        .Times(1)
        .WillOnce([&]() { sync_event2.set(); });
    EXPECT_CALL(mock3, Call())
        .Times(1)
        .WillOnce([&]() { sync_event1.set(); });

    sut.dispatch(mock1.AsStdFunction());
    ASSERT_TRUE(sync_event2.wait_for(std::chrono::seconds(10)));
    sut.dispatch(mock3.AsStdFunction());

    ASSERT_TRUE(sync_event1.wait_for(std::chrono::seconds(10)));
}
