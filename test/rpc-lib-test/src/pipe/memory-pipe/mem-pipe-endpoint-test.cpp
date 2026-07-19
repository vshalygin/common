#include <rpc-lib/pipe/memory-pipe/mem-pipe-endpoint.h>
#include <rpc-lib/pipe/memory-pipe/mem-buffers.h>
#include <common-lib/synchronization/event.h>

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

    class test_mem_pipe_endpoint
        : public mem_pipe_endpoint
    {
    public:
        test_mem_pipe_endpoint(bool is_server,
                               std::shared_ptr<mem_buffers> mem_buffers)
            : mem_pipe_endpoint(is_server, mem_buffers)
        {}
    };

    buffer create_test_data()
    {
        buffer ans(2);
        ans[0] = std::byte(10);
        ans[1] = std::byte(6);
        return ans;
    }
}

class MemPipeEndpoint
    : public Test
{
protected:
    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(2);
    }

    void TearDown() override
    {
        m_thread_pool->stop();
    }

protected:
    std::shared_ptr<thread_pool> m_thread_pool;
};

TEST_F(MemPipeEndpoint, IsConnectedAfterCreation)
{
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);

    test_mem_pipe_endpoint sut(true, buffers);

    ASSERT_TRUE(sut.is_connected());
}

TEST_F(MemPipeEndpoint, InNotConnectedAfterUnderlyingBuffersInvalidated)
{
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);

    test_mem_pipe_endpoint sut(true, buffers);
    buffers->invalidate();

    ASSERT_FALSE(sut.is_connected());
}

TEST_F(MemPipeEndpoint, SetsInvalidateCallback)
{
    event sync_event;
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    test_mem_pipe_endpoint sut(true, buffers);

    sut.set_disconnect_callback(callback.AsStdFunction());
    sut.invalidate();

    ASSERT_FALSE(sut.is_connected());
    sync_event.wait();
}

TEST_F(MemPipeEndpoint, InvalidatesUnderlyingBuffers)
{
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);

    test_mem_pipe_endpoint sut(true, buffers);
    sut.invalidate();

    ASSERT_FALSE(buffers->is_valid());
}

TEST_F(MemPipeEndpoint, WritesDataFromClientToServer)
{
    event sync_event;
    auto data = create_test_data();
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::success, BufferEq(data.data(), data.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    test_mem_pipe_endpoint server_endpoint(true, buffers);
    test_mem_pipe_endpoint client_endpoint(false, buffers);

    client_endpoint.write_async(data.copy());
    server_endpoint.read_async()
        .then(read_callback.AsStdFunction());

    sync_event.wait();
}

TEST_F(MemPipeEndpoint, WritesDataFromServerToClient)
{
    event sync_event;
    auto data = create_test_data();
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::success, BufferEq(data.data(), data.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    test_mem_pipe_endpoint server_endpoint(true, buffers);
    test_mem_pipe_endpoint client_endpoint(false, buffers);

    server_endpoint.write_async(data.copy());
    client_endpoint.read_async()
        .then(read_callback.AsStdFunction());

    sync_event.wait();
}

TEST_F(MemPipeEndpoint, ExecuteDisconnectCallbackOnDestruction)
{
    event sync_event;
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);
    auto sut = std::make_unique<test_mem_pipe_endpoint>(true, buffers);
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    sut->set_disconnect_callback(callback.AsStdFunction());

    sut.reset();

    sync_event.wait();
    Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(MemPipeEndpoint, WritesDataFromClientToServerTimeout)
{
    auto sync_event = std::make_shared<event>();
    auto data = create_test_data();
    auto pool = std::make_shared<thread_pool>(1);
    pool->post([sync_event]() { sync_event->wait(); });
    auto buffers = std::make_shared<mem_buffers>(pool);
    MockFunction<void(pipe_op_res)> write_callback;
    EXPECT_CALL(write_callback, Call(pipe_op_res::timeout))
        .Times(1);
    test_mem_pipe_endpoint client_endpoint(false, buffers);

    auto f = client_endpoint.write_async(data.copy(), std::chrono::milliseconds(0))
        .then(write_callback.AsStdFunction());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event->set();
    f.get();
    pool->stop();
}

TEST_F(MemPipeEndpoint, WritesDataFromServerToClientTimeout)
{
    auto sync_event = std::make_shared<event>();
    auto data = create_test_data();
    auto pool = std::make_shared<thread_pool>(1);
    pool->post([sync_event]() { sync_event->wait(); });
    auto buffers = std::make_shared<mem_buffers>(pool);
    MockFunction<void(pipe_op_res)> write_callback;
    EXPECT_CALL(write_callback, Call(pipe_op_res::timeout))
        .Times(1);
    test_mem_pipe_endpoint server_endpoint(true, buffers);

    auto f = server_endpoint.write_async(data.copy(), std::chrono::milliseconds(0))
        .then(write_callback.AsStdFunction());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event->set();
    f.get();
    pool->stop();
}

TEST_F(MemPipeEndpoint, ReadDataFromClient)
{
    auto sync_event = std::make_shared<event>();
    auto pool = std::make_shared<thread_pool>(1);
    pool->post([sync_event]() { sync_event->wait(); });
    auto buffers = std::make_shared<mem_buffers>(pool);
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::timeout, _))
        .Times(1);
    test_mem_pipe_endpoint server_endpoint(true, buffers);

    auto f = server_endpoint.read_async(std::chrono::milliseconds(0))
        .then(read_callback.AsStdFunction());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event->set();
    f.get();
    pool->stop();
}

TEST_F(MemPipeEndpoint, WritesDataFromServer)
{
    auto sync_event = std::make_shared<event>();
    auto pool = std::make_shared<thread_pool>(1);
    pool->post([sync_event]() { sync_event->wait(); });
    auto buffers = std::make_shared<mem_buffers>(pool);
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::timeout, _))
        .Times(1);
    test_mem_pipe_endpoint client_endpoint(false, buffers);

    auto f = client_endpoint.read_async(std::chrono::milliseconds(0))
        .then(read_callback.AsStdFunction());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event->set();
    f.get();
    pool->stop();
}
