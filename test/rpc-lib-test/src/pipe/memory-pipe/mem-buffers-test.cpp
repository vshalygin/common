#include <rpc-lib/pipe/memory-pipe/mem-buffers.h>
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

    void dummy_write_callback(pipe_op_res)
    {}

    void dummy_read_callback(pipe_op_res, buffer &&)
    {}
}

TEST(MemBuffers, IsValidAfterCreation)
{
    auto pool = std::make_shared<thread_pool>(2);
    mem_buffers sut(pool);

    ASSERT_TRUE(sut.is_valid());
}

TEST(MemBuffers, IsNotValidAfterInvalidation)
{
    auto pool = std::make_shared<thread_pool>(2);

    mem_buffers sut(pool);
    sut.invalidate();

    ASSERT_FALSE(sut.is_valid());
}

TEST(MemBuffers, WritesFromClientToServer)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto data = create_test_data();
    event sync_event;
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::success, BufferEq(data.data(), data.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    mem_buffers sut(pool);
    sut.write_async_to_server(data.copy(), &dummy_write_callback);
    sut.read_async_from_client(read_callback.AsStdFunction());

    sync_event.wait();
}

TEST(MemBuffers, WritesFromServerToClient)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto data = create_test_data();
    event sync_event;
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::success, BufferEq(data.data(), data.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    mem_buffers sut(pool);
    sut.write_async_to_client(data.copy(), &dummy_write_callback);
    sut.read_async_from_server(read_callback.AsStdFunction());

    sync_event.wait();
}

TEST(MemBuffers, SetFewInvalidateCallbacks)
{
    auto pool = std::make_shared<thread_pool>(2);
    event sync_event;
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(2)
        .WillOnce(DoDefault())
        .WillOnce([&]() { sync_event.set(); });
    mem_buffers sut(pool);
    sut.set_invalidate_callback(callback.AsStdFunction());
    sut.set_invalidate_callback(callback.AsStdFunction());
    EXPECT_EQ(sut.get_invalidate_callbacks_count(), 2);

    sut.invalidate();

    sync_event.wait();
    EXPECT_EQ(sut.get_invalidate_callbacks_count(), 0);
}

TEST(MemBuffers, SetExecuteInvalidateCallbacksImmediatelyIfInvalidated)
{
    auto pool = std::make_shared<thread_pool>(2);
    event sync_event;
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(2)
        .WillOnce(DoDefault())
        .WillOnce([&]() { sync_event.set(); });
    mem_buffers sut(pool);
    sut.invalidate();

    sut.set_invalidate_callback(callback.AsStdFunction());
    sut.set_invalidate_callback(callback.AsStdFunction());

    sync_event.wait();
    EXPECT_EQ(sut.get_invalidate_callbacks_count(), 0);
}

TEST(MemBuffers, ExecuteInvalidateCallbacksOnDestruction)
{
    auto pool = std::make_shared<thread_pool>(2);
    event sync_event;
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(2)
        .WillOnce(DoDefault())
        .WillOnce([&]() { sync_event.set(); });
    auto sut = std::make_unique<mem_buffers>(pool);
    sut->set_invalidate_callback(callback.AsStdFunction());
    sut->set_invalidate_callback(callback.AsStdFunction());

    sut.reset();
    sync_event.wait();
}
