#include <common-lib/pipe/pipe-buffer/pipe-buffer.h>
#include <common-lib/syncronization/event/event.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::cl;
using namespace testing;

TEST(PipeBuffer, IsValidAfterCreation)
{
    auto pool = std::make_shared<thread_pool>(2);

    pipe_buffer sut(pool);

    ASSERT_TRUE(sut.is_valid());
}

TEST(PipeBuffer, IsNotValidAfterInvalidation)
{
    auto pool = std::make_shared<thread_pool>(2);

    pipe_buffer sut(pool);
    sut.invalidate();

    ASSERT_FALSE(sut.is_valid());
}

TEST(PipeBuffer, AddMessageInBufferOnWriteOperationIfNoPendingReadHandlers)
{
    event sync_event;
    MockFunction<void(bool)> mock1;
    EXPECT_CALL(mock1, Call)
        .Times(1)
        .WillOnce([&](bool) { sync_event.set(); });
    auto pool = std::make_shared<thread_pool>(2);

    pipe_buffer sut(pool);
    sut.write_async("some message", mock1.AsStdFunction());
    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));

    ASSERT_EQ(sut.get_pending_messages_count(), 1);
}

TEST(PipeBuffer, CallWriteHandlerWithTrueParameterIfWritingSucceeded)
{
    event sync_event;
    MockFunction<void(bool)> mock1;
    EXPECT_CALL(mock1, Call(true))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    auto pool = std::make_shared<thread_pool>(2);
    pipe_buffer sut(pool);
    sut.write_async("some message", mock1.AsStdFunction());
    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST(PipeBuffer, CallPendingReadHandlerAfterWrite)
{
    InSequence s;
    event sync_event;
    MockFunction<void(bool)> write_mock;
    MockFunction<void(bool, std::string &&)> read_mock;
    EXPECT_CALL(write_mock, Call)
        .Times(1);
    EXPECT_CALL(read_mock, Call(true, std::string("some message")))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    auto pool = std::make_shared<thread_pool>(2);
    pipe_buffer sut(pool);
    sut.read_async(read_mock.AsStdFunction());
    sut.write_async("some message", write_mock.AsStdFunction());
    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));

    EXPECT_EQ(sut.get_pending_read_handlers_count(), 0);
    EXPECT_EQ(sut.get_pending_messages_count(), 0);
}

TEST(PipeBuffer, DoesNotCallWriteFunctorIfIsIsNotPresented)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = std::make_unique<pipe_buffer>(pool);
    sut->write_async("some message", {});
    sut.reset();
}

TEST(PipeBuffer, IfWriteHandlerThrowsItDoesNotInterruptCallingReadCallbacks)
{
    MockFunction<void(bool)> write_mock;
    MockFunction<void(bool, std::string &&)> read_mock;
    EXPECT_CALL(write_mock, Call)
        .Times(1)
        .WillOnce(Throw(std::exception()));
    EXPECT_CALL(read_mock, Call)
        .Times(1);

    auto pool = std::make_shared<thread_pool>(2);
    auto sut = std::make_unique<pipe_buffer>(pool);
    sut->read_async(read_mock.AsStdFunction());
    sut->write_async("some message", write_mock.AsStdFunction());
    sut.reset();
}

TEST(PipeBuffer, CallsReadHandlerInstantlyIfInvalidated)
{
    MockFunction<void(bool, std::string &&)> read_mock;
    EXPECT_CALL(read_mock, Call(false, _))
        .Times(1);
    auto pool = std::make_shared<thread_pool>(2);
    pipe_buffer sut(pool);

    sut.invalidate();
    sut.read_async(read_mock.AsStdFunction());

    ASSERT_EQ(sut.get_pending_read_handlers_count(), 0);
}

TEST(PipeBuffer, AddsPendingReadHandlerIfNoPendingMessages)
{
    MockFunction<void(bool, std::string &&)> read_mock;
    EXPECT_CALL(read_mock, Call)
        .Times(0);
    auto pool = std::make_shared<thread_pool>(2);
    pipe_buffer sut(pool);

    sut.read_async(read_mock.AsStdFunction());

    ASSERT_EQ(sut.get_pending_read_handlers_count(), 1);
    Mock::VerifyAndClearExpectations(&read_mock);
}

TEST(PipeBuffer, CallsReadHandlerInstantlyIfThereIsAPendingMessage)
{
    MockFunction<void(bool, std::string &&)> read_mock;
    EXPECT_CALL(read_mock, Call(true, std::string("some message")))
        .Times(1);
    auto pool = std::make_shared<thread_pool>(2);
    pipe_buffer sut(pool);

    sut.write_async("some message", {});
    sut.read_async(read_mock.AsStdFunction());

    ASSERT_EQ(sut.get_pending_read_handlers_count(), 0);
    ASSERT_EQ(sut.get_pending_messages_count(), 0);
    Mock::VerifyAndClearExpectations(&read_mock);
}

TEST(PipeBuffer, AllowsEmptyReadHandler)
{
    auto pool = std::make_shared<thread_pool>(2);
    pipe_buffer sut(pool);

    sut.invalidate();
    sut.read_async({});
}

TEST(PipeBuffer, InvalidateMethodsTriggersPendingReadCallbacks)
{
    MockFunction<void(bool, std::string &&)> read_mock;
    EXPECT_CALL(read_mock, Call(false, _))
        .Times(2);
    auto pool = std::make_shared<thread_pool>(2);
    pipe_buffer sut(pool);
    sut.read_async(read_mock.AsStdFunction());
    sut.read_async(read_mock.AsStdFunction());

    sut.invalidate();

    ASSERT_EQ(sut.get_pending_read_handlers_count(), 0);
}

TEST(PipeBuffer, DestructorTriggersPendingReadCallbacks)
{
    MockFunction<void(bool, std::string &&)> read_mock;
    EXPECT_CALL(read_mock, Call(false, _))
        .Times(2);
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = std::make_unique<pipe_buffer>(pool);
    sut->read_async(read_mock.AsStdFunction());
    sut->read_async(read_mock.AsStdFunction());

    sut.reset();
}
