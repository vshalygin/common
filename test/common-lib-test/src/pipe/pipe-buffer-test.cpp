#include <common-lib/pipe/pipe-buffer.h>
#include <common-lib/syncronization/event/event.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::cl;
using namespace testing;

namespace {
    //TODO move to test lib
    MATCHER_P2(ArrayEq, expected, size, "Arrays are equal") {
        for(size_t i = 0; i < size; ++i) {
            if(arg[i] != expected[i]) {
                return false;
            }
        }
        return true;
    }
}

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

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST(PipeBuffer, CancelsPreviousReadAsyncIfNewReadAsyncCalled)
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
    sut->read_async([](pipe_result, buffer &&) {});

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
    Mock::VerifyAndClearExpectations(&callback);
}

TEST(PipeBuffer, AnswersDisabledErrorCodeOnReadAsyncIfItIsNotEnabled)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = pipe_buffer::create(pool);
    MockFunction<void(pipe_result, buffer &&)> callback;
    EXPECT_CALL(callback, Call(pipe_result::disabled, _))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    sut->read_async(callback.AsStdFunction());

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST(PipeBuffer, CallesReadAsyncCallbackImmediatylyIfWriteQueueHasElements)
{
    buffer buf(1); buf[0] = std::byte(0x17);
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = pipe_buffer::create(pool);
    sut->enable();
    sut->write_async(buf.copy(), {});
    MockFunction<void(pipe_result, buffer &&)> callback;
    EXPECT_CALL(callback, Call(pipe_result::ok, ArrayEq(buf.data(), buf.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    sut->read_async(callback.AsStdFunction());

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST(PipeBuffer, CallesReadAsyncCallbackAfterWriteAsyncOperation)
{
    buffer buf(1); buf[0] = std::byte(0x17);
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = pipe_buffer::create(pool);
    sut->enable();
    MockFunction<void(pipe_result, buffer &&)> callback;
    EXPECT_CALL(callback, Call(pipe_result::ok, ArrayEq(buf.data(), buf.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    sut->read_async(callback.AsStdFunction());

    sut->write_async(buf.copy(), {});

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST(PipeBuffer, CallesReadAsyncCallbackSequentlyIfFewWriteOperationsPerformed)
{
    buffer buf1(1); buf1[0] = std::byte(0x17);
    buffer buf2(1); buf2[0] = std::byte(0x18);
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = pipe_buffer::create(pool);
    sut->enable();
    MockFunction<void(pipe_result, buffer &&)> callback;
    InSequence seq;
    EXPECT_CALL(callback, Call(pipe_result::ok, ArrayEq(buf1.data(), buf1.size())))
        .Times(1);
    EXPECT_CALL(callback, Call(pipe_result::ok, ArrayEq(buf2.data(), buf2.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    sut->write_async(buf1.copy(), {});
    sut->write_async(buf2.copy(), {});

    sut->read_async(callback.AsStdFunction());
    sut->read_async(callback.AsStdFunction());

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}
