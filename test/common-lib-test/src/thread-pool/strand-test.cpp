#include <common-lib/thread-pool/strand.h>
#include <common-lib/thread-pool/thread-pool.h>
#include <common-lib/syncronization/event/event.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vsh::cl;
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
    sut->post(mock1.AsStdFunction());
    sut->post(mock2.AsStdFunction());

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
    auto sut = pool.create_strand();
    sut->post(mock1.AsStdFunction());
    sut->post(mock2.AsStdFunction());

    sut.reset();
    sync_event1.set();

    sync_event2.wait_for(std::chrono::seconds(10));
}

TEST(Strand, AnswerFalseOnCheckExecutingContextIfItIsNoIn)
{
    thread_pool pool(2);
    auto sut = pool.create_strand();

    ASSERT_FALSE(sut->is_in_executing_context());
}

TEST(Strand, AnswerTrueOnCheckExecutingContextIfItIsIn)
{
    event sync_event;
    thread_pool pool(2);
    auto sut = pool.create_strand();

    sut->post([&]() {
        ASSERT_TRUE(sut->is_in_executing_context());
        sync_event.set();
    });

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST(Strand, AnswerTrueOnCheckExecutingInFunctorDestructor)
{
    class desctructor_checker
    {
    public:
        desctructor_checker(event &sync_event, strand *strand)
            : m_sync_event(sync_event)
            , m_strand(strand)
        {}

        ~desctructor_checker()
        {
            EXPECT_TRUE(m_strand->is_in_executing_context());
            m_sync_event.set();
        }

    private:
        event &m_sync_event;
        strand *m_strand;
    };

    event sync_event;
    thread_pool pool(2);
    auto sut = pool.create_strand();

    sut->post([checker = std::make_shared<desctructor_checker>(sync_event, sut.get())]() {});

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}
