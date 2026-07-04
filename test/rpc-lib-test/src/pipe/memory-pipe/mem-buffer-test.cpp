#include <rpc-lib/pipe/memory-pipe/mem-buffer.h>
#include <common-lib/synchronization/event/event.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::rpc;
using namespace vshalygin::cl;
using namespace testing;

namespace {
    //TODO move to test lib
    MATCHER_P2(BufferEq, expected, size, "Buffers are equal") {
        if(arg.size() != size) {
            return false;
        }
        for(size_t i = 0; i < size; ++i) {
            if(arg[i] != expected[i]) {
                return false;
            }
        }
        return true;
    }

    buffer create_test_data()
    {
        buffer ans(2);
        ans[0] = std::byte(10);
        ans[1] = std::byte(6);
        return ans;
    }
}

TEST(MemBuffer, IsValidJustAfterCreation)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = mem_buffer::create(pool);

    ASSERT_TRUE(sut->is_valid());
}

TEST(MemBuffer, IsNotValidJustAfterInvalidation)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = mem_buffer::create(pool);

    sut->invalidate();

    ASSERT_FALSE(sut->is_valid());
}

TEST(MemBuffer, WritesDataToBufferIfNoPendingReadCallback)
{
    auto pool = std::make_shared<thread_pool>(2);
    MockFunction<void(pipe_op_res)> callback;
    EXPECT_CALL(callback, Call(pipe_op_res::success))
        .Times(1);

    auto sut = mem_buffer::create(pool);
    sut->write_async({})
        .then(callback.AsStdFunction())
        .get();

    ASSERT_EQ(sut->get_pending_messages_count(), 1);
}

TEST(MemBuffer, ExecutePendingReadCallbackOnWrite)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto data = create_test_data();
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::success, BufferEq(data.data(), data.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    auto sut = mem_buffer::create(pool);
    sut->read_async()
        .then(read_callback.AsStdFunction());
    sut->write_async(data.copy()).get();
    
    sync_event.wait();
    EXPECT_EQ(sut->get_pending_messages_count(), 0);
    EXPECT_EQ(sut->get_pending_read_handlers_count(), 0);
}

TEST(MemBuffer, DoesNotWriteIfInvalidated)
{
    auto pool = std::make_shared<thread_pool>(2);
    MockFunction<void(pipe_op_res)> callback;
    EXPECT_CALL(callback, Call(pipe_op_res::failed))
        .Times(1);

    auto sut = mem_buffer::create(pool);
    sut->invalidate();
    sut->write_async({})
        .then(callback.AsStdFunction())
        .get();

    ASSERT_EQ(sut->get_pending_messages_count(), 0);
}

TEST(MemBuffer, AddReadHandlerOnReadAsync)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);

    auto sut = mem_buffer::create(pool);
    auto f = sut->read_async()
        .then([&](pipe_op_res, buffer &&) { sync_event.set(); });
    sut->write_async(create_test_data()).get();

    sync_event.wait();
}

TEST(MemBuffer, DoesNotReadAsyncIfInvalid)
{
    auto pool = std::make_shared<thread_pool>(2);
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::failed, _))
        .Times(1);

    auto sut = mem_buffer::create(pool);
    sut->invalidate();
    sut->read_async()
        .then(read_callback.AsStdFunction())
        .get();

    ASSERT_TRUE(sut->get_pending_read_handlers_count() == 0);
}

TEST(MemBuffer, ReadsWrittenData)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto data = create_test_data();
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::success, BufferEq(data.data(), data.size())))
        .Times(1);

    auto sut = mem_buffer::create(pool);
    sut->write_async(data.copy());
    sut->read_async()
        .then(read_callback.AsStdFunction())
        .get();

    ASSERT_TRUE(sut->get_pending_read_handlers_count() == 0);
}

TEST(MemBuffer, ExecutePendingReadCallbacksOnInvalidation)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::canceled, _))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    auto sut = mem_buffer::create(pool);
    sut->read_async()
        .then(read_callback.AsStdFunction());
    while(sut->get_pending_read_handlers_count() != 1) {}

    sut->invalidate();

    sync_event.wait();
    ASSERT_TRUE(sut->get_pending_read_handlers_count() == 0);
}

TEST(MemBuffer, ClearsPendingMessagesOnInvalidation)
{
    auto pool = std::make_shared<thread_pool>(2);

    auto sut = mem_buffer::create(pool);
    sut->write_async({}).get();
    sut->invalidate();

    ASSERT_TRUE(sut->get_pending_messages_count() == 0);
}

TEST(MemBuffer, ExecutePendingReadCallbacksOnDestruction)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::canceled, _))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    auto sut = mem_buffer::create(pool);
    sut->read_async()
        .then(read_callback.AsStdFunction());
    while(sut->get_pending_read_handlers_count() != 1) {}
    sut.reset();

    sync_event.wait();
    Mock::VerifyAndClearExpectations(&read_callback);
}
