#include <rpc-lib/pipe/memory-pipe/mem-buffer.h>

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

TEST(MemBuffer, IsValidJustAfterCreation)
{
    auto pool = std::make_shared<thread_pool>(2);
    mem_buffer sut(pool);

    ASSERT_TRUE(sut.is_valid());
}

TEST(MemBuffer, IsNotValidJustAfterInvalidation)
{
    auto pool = std::make_shared<thread_pool>(2);
    mem_buffer sut(pool);

    sut.invalidate();

    ASSERT_FALSE(sut.is_valid());
}

TEST(MemBuffer, WritesDataToBufferIfNoPendingReadCallback)
{
    auto pool = std::make_shared<thread_pool>(2);
    MockFunction<void(pipe_op_res)> callback;
    EXPECT_CALL(callback, Call(pipe_op_res::success))
        .Times(1);

    mem_buffer sut(pool);
    sut.write_async({}, callback.AsStdFunction());

    ASSERT_EQ(sut.get_pending_messages_count(), 1);
}

TEST(MemBuffer, ExecutePendingReadCallbackOnWrite)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto data = create_test_data();
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::success, BufferEq(data.data(), data.size())))
        .Times(1);

    mem_buffer sut(pool);
    sut.read_async(read_callback.AsStdFunction());
    sut.write_async(data.copy(), &dummy_write_callback);

    EXPECT_EQ(sut.get_pending_messages_count(), 0);
    EXPECT_EQ(sut.get_pending_read_handlers_count(), 0);
}

TEST(MemBuffer, DoesNotWriteIfInvalidated)
{
    auto pool = std::make_shared<thread_pool>(2);
    MockFunction<void(pipe_op_res)> callback;
    EXPECT_CALL(callback, Call(pipe_op_res::failed))
        .Times(1);

    mem_buffer sut(pool);
    sut.invalidate();
    sut.write_async({}, callback.AsStdFunction());

    ASSERT_EQ(sut.get_pending_messages_count(), 0);
}

TEST(MemBuffer, AddReadHandlerOnReadAsync)
{
    auto pool = std::make_shared<thread_pool>(2);

    mem_buffer sut(pool);
    sut.read_async(&dummy_read_callback);

    ASSERT_TRUE(sut.get_pending_read_handlers_count() == 1);
}

TEST(MemBuffer, DoesNotReadAsyncIfInvalid)
{
    auto pool = std::make_shared<thread_pool>(2);
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::failed, _))
        .Times(1);

    mem_buffer sut(pool);
    sut.invalidate();
    sut.read_async(read_callback.AsStdFunction());

    ASSERT_TRUE(sut.get_pending_read_handlers_count() == 0);
}

TEST(MemBuffer, ReadsWrittenData)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto data = create_test_data();
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::success, BufferEq(data.data(), data.size())))
        .Times(1);

    mem_buffer sut(pool);
    sut.write_async(data.copy(), &dummy_write_callback);
    sut.read_async(read_callback.AsStdFunction());

    ASSERT_TRUE(sut.get_pending_read_handlers_count() == 0);
}

TEST(MemBuffer, ExecutePendingReadCallbacksOnInvalidation)
{
    auto pool = std::make_shared<thread_pool>(2);
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::canceled, _))
        .Times(1);

    mem_buffer sut(pool);
    sut.read_async(read_callback.AsStdFunction());
    sut.invalidate();

    ASSERT_TRUE(sut.get_pending_read_handlers_count() == 0);
}

TEST(MemBuffer, ClearsPendingMessagesOnInvalidation)
{
    auto pool = std::make_shared<thread_pool>(2);

    mem_buffer sut(pool);
    sut.write_async({}, &dummy_write_callback);
    sut.invalidate();

    ASSERT_TRUE(sut.get_pending_messages_count() == 0);
}

TEST(MemBuffer, ExecutePendingReadCallbacksOnDestruction)
{
    auto pool = std::make_shared<thread_pool>(2);
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::canceled, _))
        .Times(1);

    auto sut = std::make_unique<mem_buffer>(pool);
    sut->read_async(read_callback.AsStdFunction());
    sut.reset();

    Mock::VerifyAndClearExpectations(&read_callback);
}
