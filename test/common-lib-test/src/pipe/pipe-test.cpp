#include <common-lib/pipe/pipe.h>
#include <common-lib/syncronization/event/event.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

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
        : public pipe
    {
    public:
        explicit test_pipe(bool is_server)
            : pipe(is_server)
        {}

        using pipe::set_buffers;
    };
}

class Pipe
    : public Test
{
protected:
    void SetUp() override
    {
        thread_pool_ = std::make_shared<thread_pool>(2);
    }

    std::shared_ptr<thread_pool> thread_pool_;
};

class ServerPipe
    : public Pipe
{
protected:
    void SetUp() override
    {
        Pipe::SetUp();
        sut_ = std::make_unique<test_pipe>(true);
        buffers_ = std::make_shared<pipe_buffers>(thread_pool_);
    }

    std::unique_ptr<test_pipe> sut_;
    std::shared_ptr<pipe_buffers> buffers_;
};

class ClientPipe
    : public Pipe
{
protected:
    void SetUp() override
    {
        Pipe::SetUp();
        sut_ = std::make_unique<test_pipe>(false);
        buffers_ = std::make_shared<pipe_buffers>(thread_pool_);
    }

    std::unique_ptr<test_pipe> sut_;
    std::shared_ptr<pipe_buffers> buffers_;
};

TEST_F(Pipe, IsNotConnectedByDefault)
{
    test_pipe sut(true);

    ASSERT_FALSE(sut.is_connected());
}

TEST_F(Pipe, IsConnectedAfterAddingBuffers)
{
    test_pipe sut(true);
    sut.set_buffers(std::make_shared<pipe_buffers>(thread_pool_));

    ASSERT_TRUE(sut.is_connected());
}

TEST_F(Pipe, IsNotConnectedAfterBufferInvalidated)
{
    auto buffers = std::make_shared<pipe_buffers>(thread_pool_);
    test_pipe sut(true);
    sut.set_buffers(buffers);

    buffers->client_to_server.invalidate();
    buffers->server_to_client.invalidate();

    ASSERT_FALSE(sut.is_connected());
}

TEST_F(Pipe, InvalidatesHoldingBuffersBeforeDestruction)
{
    auto buffers = std::make_shared<pipe_buffers>(thread_pool_);

    {
        test_pipe sut(true);
        sut.set_buffers(buffers);
    }

    ASSERT_FALSE(buffers->client_to_server.is_valid());
    ASSERT_FALSE(buffers->server_to_client.is_valid());
}

TEST_F(Pipe, ReturnsFalseIfNoConnectionAfterWaiting)
{
    test_pipe sut(true);

    ASSERT_FALSE(sut.wait_connect_for(std::chrono::microseconds(1)));
}

TEST_F(Pipe, WaitsConnection)
{
    auto buffers = std::make_shared<pipe_buffers>(thread_pool_);
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

TEST_F(Pipe, CancelsWaiting)
{
    auto buffers = std::make_shared<pipe_buffers>(thread_pool_);
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

TEST_F(Pipe, WaitsConnectionTimed)
{
    auto buffers = std::make_shared<pipe_buffers>(thread_pool_);
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

TEST_F(Pipe, CancelsTimedWaiting)
{
    auto buffers = std::make_shared<pipe_buffers>(thread_pool_);
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

TEST_F(Pipe, InvalidateBuffers)
{
    auto buffers = std::make_shared<pipe_buffers>(thread_pool_);
    test_pipe sut(true);
    sut.set_buffers(buffers);

    sut.invalidate();

    EXPECT_FALSE(buffers->client_to_server.is_valid());
    EXPECT_FALSE(buffers->server_to_client.is_valid());
}

TEST_F(Pipe, DoesNothingOnInvalidationIfNoBufers)
{
    test_pipe sut(true);

    sut.invalidate();
}

TEST_F(ServerPipe, WriteAsyncReturnsFalseIfNoBuffers)
{
    ASSERT_FALSE(sut_->write_async({}, {}));
}

TEST_F(ServerPipe, WriteAsyncReturnsFalseIfOutputBufferInvalid)
{
    buffers_->server_to_client.invalidate();
    sut_->set_buffers(buffers_);

    ASSERT_FALSE(sut_->write_async({}, {}));
}

TEST_F(ServerPipe, WriteAsyncWritesMessageInOutputBuffer)
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

TEST_F(ServerPipe, ReadAsyncReturnsFalseIfNoBuffers)
{
    ASSERT_FALSE(sut_->read_async({}));
}

TEST_F(ServerPipe, ReadAsyncReturnsFalseIfInputBufferInvalid)
{
    buffers_->client_to_server.invalidate();
    sut_->set_buffers(buffers_);

    ASSERT_FALSE(sut_->read_async({}));
}

TEST_F(ServerPipe, ReadAsyncReadsMessageFromInputBuffer)
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

TEST_F(ServerPipe, TryToWriteForReturnsFalseIfNoBuffers)
{
    ASSERT_FALSE(sut_->try_to_write_for({}, std::chrono::seconds(10)));
}

TEST_F(ServerPipe, TryToWriteForReturnsFalseIfOutputBufferInvalid)
{
    buffers_->server_to_client.invalidate();
    sut_->set_buffers(buffers_);

    ASSERT_FALSE(sut_->try_to_write_for({}, std::chrono::seconds(10)));
}

TEST_F(ServerPipe, TryToWriteForWritesMessageInOutputBuffer)
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

TEST_F(ServerPipe, TryToReadForReturnsFalseIfNoBuffers)
{
    ASSERT_FALSE(sut_->try_to_read_for({}));
}

TEST_F(ServerPipe, TryToReadForReturnsFalseIfInputBufferInvalid)
{
    buffers_->client_to_server.invalidate();
    sut_->set_buffers(buffers_);

    ASSERT_FALSE(sut_->try_to_read_for({}));
}

TEST_F(ServerPipe, TryToReadForReadsMessageFromInputBuffer)
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

TEST_F(ClientPipe, WriteAsyncReturnsFalseIfNoBuffers)
{
    ASSERT_FALSE(sut_->write_async({}, {}));
}

TEST_F(ClientPipe, WriteAsyncReturnsFalseIfOutputBufferInvalid)
{
    buffers_->client_to_server.invalidate();
    sut_->set_buffers(buffers_);

    ASSERT_FALSE(sut_->write_async({}, {}));
}

TEST_F(ClientPipe, WriteAsyncWritesMessageInOutputBuffer)
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

TEST_F(ClientPipe, ReadAsyncReturnsFalseIfNoBuffers)
{
    ASSERT_FALSE(sut_->read_async({}));
}

TEST_F(ClientPipe, ReadAsyncReturnsFalseIfInputBufferInvalid)
{
    buffers_->server_to_client.invalidate();
    sut_->set_buffers(buffers_);

    ASSERT_FALSE(sut_->read_async({}));
}

TEST_F(ClientPipe, ReadAsyncReadsMessageFromInputBuffer)
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
