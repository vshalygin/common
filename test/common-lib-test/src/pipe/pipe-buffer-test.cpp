#include <common-lib/pipe/pipe-buffer.h>
#include <common-lib/syncronization/event/event.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vsh::cl;
using namespace testing;

TEST(PipeBuffer, IsDisabledAfterCreation)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = pipe_buffer::create(pool);

    auto ans = sut->is_enabled();

    ASSERT_FALSE(ans);
}

TEST(PipeBuffer, IsEnabledAfterEnable)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = pipe_buffer::create(pool);

    sut->enable();

    ASSERT_TRUE(sut->is_enabled());
}

TEST(PipeBuffer, IsDisabledAfterEnableAndDisable)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = pipe_buffer::create(pool);

    sut->enable();
    sut->disable();

    ASSERT_FALSE(sut->is_enabled());
}

TEST(PipeBuffer, AnswersZeroMessageQueueCountAfterCreation)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = pipe_buffer::create(pool);

    auto ans = sut->get_message_queue_count();

    ASSERT_EQ(ans, 0);
}

TEST(PipeBuffer, WriteCallbackCallsWithDisabledCodeIfBufferIsDisabled)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = pipe_buffer::create(pool);

    MockFunction<void(pipe_result)> callback;
    EXPECT_CALL(callback, Call(pipe_result::disabled))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    sut->write_async({}, callback.AsStdFunction());

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
    ASSERT_EQ(sut->get_message_queue_count(), 0);
}

TEST(PipeBuffer, AddsMessageToQueueOnWriteWhenEnabled)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = pipe_buffer::create(pool);
    sut->enable();

    MockFunction<void(pipe_result)> callback;
    EXPECT_CALL(callback, Call(pipe_result::ok))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    sut->write_async({}, callback.AsStdFunction());

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
    ASSERT_EQ(sut->get_message_queue_count(), 1);
}

TEST(PipeBuffer, ClearsMesssageQueueAfterDisabling)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = pipe_buffer::create(pool);
    sut->enable();
    MockFunction<void(pipe_result)> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    sut->write_async({}, callback.AsStdFunction());
    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));

    sut->disable();
    ASSERT_EQ(sut->get_message_queue_count(), 0);
}

TEST(PipeBuffer, CancelsActiveAsyncReadOnDestruction)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = pipe_buffer::create(pool);
    sut->enable();
    MockFunction<void(pipe_result, buffer &&)> callback;
    EXPECT_CALL(callback, Call(pipe_result::canceled, _))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    sut->read_async(callback.AsStdFunction());
    sut.reset();

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(1000)));
}
