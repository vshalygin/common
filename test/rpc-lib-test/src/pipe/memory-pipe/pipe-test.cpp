#include <rpc-lib/pipe/memory-pipe/mem-pipe.h>
#include <common-lib/syncronization/event/event.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::rpc;
using namespace vshalygin::cl;
using namespace testing;

namespace {
    //TODO move to test lib
    MATCHER_P2(BufferEq, expected, size, "Arrays are equal") {
        for(size_t i = 0; i < size; ++i) {
            if(arg[i] != expected[i]) {
                return false;
            }
        }
        return true;
    }
}

namespace {
    class test_pipe
        : public mem_pipe
    {
    public:
        explicit test_pipe(bool is_server)
            : mem_pipe(is_server)
        {}

        using mem_pipe::set_buffers;
    };
}

class MemPipe
    : public Test
{
protected:
    void SetUp() override
    {
        thread_pool_ = std::make_shared<thread_pool>(2);
    }

    std::shared_ptr<thread_pool> thread_pool_;
};

class ServerMemPipe
    : public MemPipe
{
protected:
    void SetUp() override
    {
        MemPipe::SetUp();
        sut_ = std::make_unique<test_pipe>(true);
        buffers_ = std::make_shared<mem_buffers>(thread_pool_);
    }

    std::unique_ptr<test_pipe> sut_;
    std::shared_ptr<mem_buffers> buffers_;
};

class ClientMemPipe
    : public MemPipe
{
protected:
    void SetUp() override
    {
        MemPipe::SetUp();
        sut_ = std::make_unique<test_pipe>(false);
        buffers_ = std::make_shared<mem_buffers>(thread_pool_);
    }

    std::unique_ptr<test_pipe> sut_;
    std::shared_ptr<mem_buffers> buffers_;
};

TEST_F(MemPipe, IsNotConnectedByDefault)
{
    test_pipe sut(true);

    ASSERT_FALSE(sut.is_connected());
}

TEST_F(MemPipe, IsConnectedAfterAddingBuffers)
{
    test_pipe sut(true);
    sut.set_buffers(std::make_shared<mem_buffers>(thread_pool_));

    ASSERT_TRUE(sut.is_connected());
}

TEST_F(MemPipe, IsNotConnectedAfterBufferInvalidated)
{
    auto buffers = std::make_shared<mem_buffers>(thread_pool_);
    test_pipe sut(true);
    sut.set_buffers(buffers);

    buffers->client_to_server.invalidate();
    buffers->server_to_client.invalidate();

    ASSERT_FALSE(sut.is_connected());
}

TEST_F(MemPipe, InvalidatesHoldingBuffersBeforeDestruction)
{
    auto buffers = std::make_shared<mem_buffers>(thread_pool_);

    {
        test_pipe sut(true);
        sut.set_buffers(buffers);
    }

    ASSERT_FALSE(buffers->client_to_server.is_valid());
    ASSERT_FALSE(buffers->server_to_client.is_valid());
}

TEST_F(MemPipe, ReturnsFalseIfNoConnectionAfterWaiting)
{
    test_pipe sut(true);

    ASSERT_FALSE(sut.wait_connect_for(std::chrono::microseconds(1)));
}

TEST_F(MemPipe, WaitsConnection)
{
    auto buffers = std::make_shared<mem_buffers>(thread_pool_);
    event sync_event;
    test_pipe sut(true);

    thread_pool_->post([&]() {
        sync_event.set();
        sut.wait_connect();
    });
    sync_event.wait();
    std::this_thread::sleep_for(std::chrono::microseconds(10));

    sut.set_buffers(buffers);
}

TEST_F(MemPipe, CancelsWaiting)
{
    auto buffers = std::make_shared<mem_buffers>(thread_pool_);
    event sync_event1;
    event sync_event2;
    test_pipe sut(true);

    thread_pool_->post([&]() {
        sync_event1.set();
        sut.wait_connect();
        sync_event2.set();
    });
    sync_event1.wait();
    std::this_thread::sleep_for(std::chrono::microseconds(10));

    sut.invalidate();
    sync_event2.wait();

    ASSERT_FALSE(sut.is_connected());
}

TEST_F(MemPipe, WaitsConnectionTimed)
{
    auto buffers = std::make_shared<mem_buffers>(thread_pool_);
    event sync_event;
    test_pipe sut(true);

    thread_pool_->post([&]() {
        sync_event.set();
        ASSERT_TRUE(sut.wait_connect_for(std::chrono::seconds(10)));
    });
    sync_event.wait();
    std::this_thread::sleep_for(std::chrono::microseconds(10));

    sut.set_buffers(buffers);
}

TEST_F(MemPipe, CancelsTimedWaiting)
{
    auto buffers = std::make_shared<mem_buffers>(thread_pool_);
    event sync_event1;
    event sync_event2;
    test_pipe sut(true);

    thread_pool_->post([&]() {
        sync_event1.set();
        ASSERT_TRUE(sut.wait_connect_for(std::chrono::seconds(10)));
        sync_event2.set();
    });
    sync_event1.wait();
    std::this_thread::sleep_for(std::chrono::microseconds(10));

    sut.invalidate();
    sync_event2.wait();

    ASSERT_FALSE(sut.is_connected());
}

TEST_F(MemPipe, InvalidateBuffers)
{
    auto buffers = std::make_shared<mem_buffers>(thread_pool_);
    test_pipe sut(true);
    sut.set_buffers(buffers);

    sut.invalidate();

    EXPECT_FALSE(buffers->client_to_server.is_valid());
    EXPECT_FALSE(buffers->server_to_client.is_valid());
}

TEST_F(MemPipe, DoesNothingOnInvalidationIfNoBufers)
{
    test_pipe sut(true);

    sut.invalidate();
}

TEST_F(ServerMemPipe, WriteAsyncReturnsFalseIfNoBuffers)
{
    ASSERT_FALSE(sut_->write_async({}, {}));
}

TEST_F(ServerMemPipe, WriteAsyncReturnsFalseIfOutputBufferInvalid)
{
    buffers_->server_to_client.invalidate();
    sut_->set_buffers(buffers_);

    ASSERT_FALSE(sut_->write_async({}, {}));
}

TEST_F(ServerMemPipe, WriteAsyncWritesMessageInOutputBuffer)
{
    buffer buf(2);
    buf[0] = (std::byte)0x1;
    buf[1] = (std::byte)0x2;
    MockFunction<void(pipe_op_res)> write_mock;
    MockFunction<void(pipe_op_res, buffer &&)> read_mock;
    EXPECT_CALL(write_mock, Call(pipe_op_res::success))
        .Times(1);
    EXPECT_CALL(read_mock, Call(pipe_op_res::success, BufferEq(buf.data(), buf.size())))
        .Times(1);

    sut_->set_buffers(buffers_);
    EXPECT_TRUE(sut_->write_async(buf.copy(), write_mock.AsStdFunction()));

    EXPECT_TRUE(buffers_->server_to_client.get_pending_messages_count() == 1);
    buffers_->server_to_client.read_async(read_mock.AsStdFunction());
    sut_.reset();
    buffers_.reset();
}

TEST_F(ServerMemPipe, ReadAsyncReturnsFalseIfNoBuffers)
{
    ASSERT_FALSE(sut_->read_async({}));
}

TEST_F(ServerMemPipe, ReadAsyncReturnsFalseIfInputBufferInvalid)
{
    buffers_->client_to_server.invalidate();
    sut_->set_buffers(buffers_);

    ASSERT_FALSE(sut_->read_async({}));
}

TEST_F(ServerMemPipe, ReadAsyncReadsMessageFromInputBuffer)
{
    buffer buf(2);
    buf[0] = (std::byte)0x1;
    buf[1] = (std::byte)0x2;
    MockFunction<void(pipe_op_res, buffer &&)> read_mock;
    EXPECT_CALL(read_mock, Call(pipe_op_res::success, BufferEq(buf.data(), buf.size())))
        .Times(1);

    sut_->set_buffers(buffers_);
    EXPECT_TRUE(sut_->read_async(read_mock.AsStdFunction()));

    EXPECT_TRUE(buffers_->client_to_server.get_pending_read_handlers_count() == 1);
    buffers_->client_to_server.write_async(buf.copy(), {});
    sut_.reset();
    buffers_.reset();
}

TEST_F(ServerMemPipe, TryToWriteForReturnsFalseIfNoBuffers)
{
    ASSERT_FALSE(sut_->try_to_write_for({}, std::chrono::seconds(10)));
}

TEST_F(ServerMemPipe, TryToWriteForReturnsFalseIfOutputBufferInvalid)
{
    buffers_->server_to_client.invalidate();
    sut_->set_buffers(buffers_);

    ASSERT_FALSE(sut_->try_to_write_for({}, std::chrono::seconds(10)));
}

TEST_F(ServerMemPipe, TryToWriteForWritesMessageInOutputBuffer)
{
    buffer buf(2);
    buf[0] = (std::byte)0x1;
    buf[1] = (std::byte)0x2;
    MockFunction<void(pipe_op_res, buffer &&)> read_mock;
    EXPECT_CALL(read_mock, Call(pipe_op_res::success, BufferEq(buf.data(), buf.size())))
        .Times(1);
    sut_->set_buffers(buffers_);
    
    auto res = sut_->try_to_write_for(buf.copy(), std::chrono::seconds(10));

    ASSERT_TRUE(res);
    buffers_->server_to_client.read_async(read_mock.AsStdFunction());

    sut_.reset();
    buffers_.reset();
}

TEST_F(ServerMemPipe, TryToReadForReturnsFalseIfNoBuffers)
{
    ASSERT_FALSE(sut_->try_to_read_for({}));
}

TEST_F(ServerMemPipe, TryToReadForReturnsFalseIfInputBufferInvalid)
{
    buffers_->client_to_server.invalidate();
    sut_->set_buffers(buffers_);

    ASSERT_FALSE(sut_->try_to_read_for({}));
}

TEST_F(ServerMemPipe, TryToReadForReadsMessageFromInputBuffer)
{
    buffer buf(2);
    buf[0] = (std::byte)0x1;
    buf[1] = (std::byte)0x2;
    MockFunction<void(pipe_op_res, buffer &&)> read_mock;
    sut_->set_buffers(buffers_);
    buffers_->client_to_server.write_async(buf.copy(), {});

    auto res = sut_->try_to_read_for(std::chrono::seconds(10));
    ASSERT_TRUE(res && *res == buf);

    sut_.reset();
    buffers_.reset();
}

TEST_F(ClientMemPipe, WriteAsyncReturnsFalseIfNoBuffers)
{
    ASSERT_FALSE(sut_->write_async({}, {}));
}

TEST_F(ClientMemPipe, WriteAsyncReturnsFalseIfOutputBufferInvalid)
{
    buffers_->client_to_server.invalidate();
    sut_->set_buffers(buffers_);

    ASSERT_FALSE(sut_->write_async({}, {}));
}

TEST_F(ClientMemPipe, WriteAsyncWritesMessageInOutputBuffer)
{
    buffer buf(2);
    buf[0] = (std::byte)0x1;
    buf[1] = (std::byte)0x2;
    MockFunction<void(pipe_op_res)> write_mock;
    MockFunction<void(pipe_op_res, buffer &&)> read_mock;
    EXPECT_CALL(write_mock, Call(pipe_op_res::success))
        .Times(1);
    EXPECT_CALL(read_mock, Call(pipe_op_res::success, BufferEq(buf.data(), buf.size())))
        .Times(1);

    sut_->set_buffers(buffers_);
    EXPECT_TRUE(sut_->write_async(buf.copy(), write_mock.AsStdFunction()));

    EXPECT_TRUE(buffers_->client_to_server.get_pending_messages_count() == 1);
    buffers_->client_to_server.read_async(read_mock.AsStdFunction());
    sut_.reset();
    buffers_.reset();
}

TEST_F(ClientMemPipe, ReadAsyncReturnsFalseIfNoBuffers)
{
    ASSERT_FALSE(sut_->read_async({}));
}

TEST_F(ClientMemPipe, ReadAsyncReturnsFalseIfInputBufferInvalid)
{
    buffers_->server_to_client.invalidate();
    sut_->set_buffers(buffers_);

    ASSERT_FALSE(sut_->read_async({}));
}

TEST_F(ClientMemPipe, ReadAsyncReadsMessageFromInputBuffer)
{
    buffer buf(2);
    buf[0] = (std::byte)0x1;
    buf[1] = (std::byte)0x2;
    MockFunction<void(pipe_op_res, buffer &&)> read_mock;
    EXPECT_CALL(read_mock, Call(pipe_op_res::success, BufferEq(buf.data(), buf.size())))
        .Times(1);

    sut_->set_buffers(buffers_);
    EXPECT_TRUE(sut_->read_async(read_mock.AsStdFunction()));

    EXPECT_TRUE(buffers_->server_to_client.get_pending_read_handlers_count() == 1);
    buffers_->server_to_client.write_async(buf.copy(), {});
    sut_.reset();
    buffers_.reset();
}
