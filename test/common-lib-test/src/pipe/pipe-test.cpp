#include <common-lib/pipe/pipe.h>
#include <common-lib/syncronization/event/event.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::cl;
using namespace testing;

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
        ASSERT_TRUE(sut.wait_connect_for(std::chrono::seconds(10)));
    });
    sync_event.wait();
    std::this_thread::sleep_for(std::chrono::microseconds(10));

    sut.set_buffers(buffers);
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
    MockFunction<void(bool)> write_mock;
    MockFunction<void(bool, std::string &&)> read_mock;
    EXPECT_CALL(write_mock, Call(true))
        .Times(1);
    EXPECT_CALL(read_mock, Call(true, std::string("some message")))
        .Times(1);

    sut_->set_buffers(buffers_);
    EXPECT_TRUE(sut_->write_async("some message", write_mock.AsStdFunction()));

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
    MockFunction<void(bool, std::string &&)> read_mock;
    EXPECT_CALL(read_mock, Call(true, std::string("some message")))
        .Times(1);

    sut_->set_buffers(buffers_);
    EXPECT_TRUE(sut_->read_async(read_mock.AsStdFunction()));

    EXPECT_TRUE(buffers_->client_to_server.get_pending_read_handlers_count() == 1);
    buffers_->client_to_server.write_async("some message", {});
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
    MockFunction<void(bool)> write_mock;
    MockFunction<void(bool, std::string &&)> read_mock;
    EXPECT_CALL(write_mock, Call(true))
        .Times(1);
    EXPECT_CALL(read_mock, Call(true, std::string("some message")))
        .Times(1);

    sut_->set_buffers(buffers_);
    EXPECT_TRUE(sut_->write_async("some message", write_mock.AsStdFunction()));

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
    MockFunction<void(bool, std::string &&)> read_mock;
    EXPECT_CALL(read_mock, Call(true, std::string("some message")))
        .Times(1);

    sut_->set_buffers(buffers_);
    EXPECT_TRUE(sut_->read_async(read_mock.AsStdFunction()));

    EXPECT_TRUE(buffers_->server_to_client.get_pending_read_handlers_count() == 1);
    buffers_->server_to_client.write_async("some message", {});
    sut_.reset();
    buffers_.reset();
}
