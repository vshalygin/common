#include <rpc-lib/pipe/memory-pipe/mem-pipe-endpoint.h>
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

    class test_mem_pipe_endpoint
        : public mem_pipe_endpoint
    {
    public:
        test_mem_pipe_endpoint(bool is_server,
                               std::shared_ptr<thread_pool> thread_pool)
            : mem_pipe_endpoint(is_server, thread_pool)
        {}

        void set_buffers(std::shared_ptr<mem_buffers> mem_buffers)
        {
            mem_pipe_endpoint::set_buffers(std::move(mem_buffers));
        }
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

protected:
    std::shared_ptr<thread_pool> m_thread_pool;
};

TEST_F(MemPipeEndpoint, InNotConnectedAfterCreation)
{
    test_mem_pipe_endpoint sut(true, m_thread_pool);

    ASSERT_FALSE(sut.is_connected());
}

TEST_F(MemPipeEndpoint, InConnectedAfterSettingBuffers)
{
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    sut.set_buffers(buffers);

    ASSERT_TRUE(sut.is_connected());
}

TEST_F(MemPipeEndpoint, InNotConnectedAfterUnderlyingBuffersInvalidated)
{
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    sut.set_buffers(buffers);
    buffers->invalidate();

    ASSERT_FALSE(sut.is_connected());
}

TEST_F(MemPipeEndpoint, SetsInvalidateCallbackAfterConnect)
{
    event sync_event;
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    sut.set_buffers(buffers);
    sut.set_disconnect_callback(callback.AsStdFunction());
    sut.invalidate();

    ASSERT_FALSE(sut.is_connected());
    sync_event.wait();
}

TEST_F(MemPipeEndpoint, SetsInvalidateCallbackBeforeConnect)
{
    event sync_event;
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    sut.set_disconnect_callback(callback.AsStdFunction());
    sut.set_buffers(buffers);
    sut.invalidate();

    ASSERT_FALSE(sut.is_connected());
    sync_event.wait();
}

TEST_F(MemPipeEndpoint, InvalidatesSettingBuffersIfWasInvalidatedBefore)
{
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    sut.invalidate();
    sut.set_buffers(buffers);

    ASSERT_FALSE(buffers->is_valid());
}

TEST_F(MemPipeEndpoint, InvalidatesUnderlyingBuffers)
{
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    sut.set_buffers(buffers);
    sut.invalidate();

    ASSERT_FALSE(buffers->is_valid());
}

TEST_F(MemPipeEndpoint, ExecuteDisconnectCallbackOnlyOnceIfBuffersWereNotSet)
{
    event sync_event;
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    sut.set_disconnect_callback(callback.AsStdFunction());
    sut.invalidate();
    sut.invalidate();

    sync_event.wait();
}

TEST_F(MemPipeEndpoint, WaitsConnect)
{
    event sync_event;
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    m_thread_pool->post([&]() {
        auto res = sut.wait_connect();
        EXPECT_EQ(res, pipe_wait_res::connected);
        sync_event.set();
    });

    sut.set_buffers(buffers);

    sync_event.wait();
}

TEST_F(MemPipeEndpoint, StopWaitingConnectIfItIsAlreadyConnected)
{
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    sut.set_buffers(buffers);

    EXPECT_EQ(sut.wait_connect(), pipe_wait_res::connected);
}

TEST_F(MemPipeEndpoint, StopsWaitingConnectIfInvalidated)
{
    event sync_event;

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    m_thread_pool->post([&]() {
        auto res = sut.wait_connect();
        EXPECT_EQ(res, pipe_wait_res::invalidated);
        sync_event.set();
    });

    sut.invalidate();

    sync_event.wait();
}

TEST_F(MemPipeEndpoint, StopsWaitingConnectIfItIsInvalidated)
{
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    sut.invalidate();

    EXPECT_EQ(sut.wait_connect(), pipe_wait_res::invalidated);
}

TEST_F(MemPipeEndpoint, WaitsConnectSuccessfullyBeforeTimeout)
{
    event sync_event;
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    m_thread_pool->post([&]() {
        auto res = sut.wait_connect_for(std::chrono::seconds(100));
        EXPECT_EQ(res, pipe_wait_res::connected);
        sync_event.set();
    });

    sut.set_buffers(buffers);

    sync_event.wait();
}

TEST_F(MemPipeEndpoint, CancelsWaitingOnTimeout)
{
    test_mem_pipe_endpoint sut(true, m_thread_pool);

    auto res = sut.wait_connect_for(std::chrono::microseconds(1));
    EXPECT_EQ(res, pipe_wait_res::timeout);
}

TEST_F(MemPipeEndpoint, StopWaitingConnectUntilTimeoutIfItIsAlreadyConnected)
{
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    sut.set_buffers(buffers);

    EXPECT_EQ(sut.wait_connect_for(std::chrono::seconds(100)), pipe_wait_res::connected);
}

TEST_F(MemPipeEndpoint, StopsWaitingConnectUntilTimeoutIfInvalidated)
{
    event sync_event;

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    m_thread_pool->post([&]() {
        auto res = sut.wait_connect_for(std::chrono::seconds(100));
        EXPECT_EQ(res, pipe_wait_res::invalidated);
        sync_event.set();
    });

    sut.invalidate();
    sync_event.wait();
}

TEST_F(MemPipeEndpoint, StopsWaitingConnectUntilTimeoutIfItIsInvalidated)
{
    auto buffers = std::make_shared<mem_buffers>(m_thread_pool);

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    sut.invalidate();

    EXPECT_EQ(sut.wait_connect_for(std::chrono::seconds(100)), pipe_wait_res::invalidated);
}

TEST_F(MemPipeEndpoint, ExecuteWriteCallbackWithFailedCodeIfBuffersWereNotSet)
{
    event sync_event;

    MockFunction<void(pipe_op_res)> callback;
    EXPECT_CALL(callback, Call(pipe_op_res::failed))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    test_mem_pipe_endpoint sut(true, m_thread_pool);
    sut.write_async({})
        .then(callback.AsStdFunction());

    sync_event.wait();
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
    test_mem_pipe_endpoint server_endpoint(true, m_thread_pool);
    test_mem_pipe_endpoint client_endpoint(false, m_thread_pool);
    server_endpoint.set_buffers(buffers);
    client_endpoint.set_buffers(buffers);

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
    test_mem_pipe_endpoint server_endpoint(true, m_thread_pool);
    test_mem_pipe_endpoint client_endpoint(false, m_thread_pool);
    server_endpoint.set_buffers(buffers);
    client_endpoint.set_buffers(buffers);

    server_endpoint.write_async(data.copy());
    client_endpoint.read_async()
        .then(read_callback.AsStdFunction());

    sync_event.wait();
}

TEST_F(MemPipeEndpoint, ExecuteDisconnectCallbackOnDestruction)
{
    event sync_event;
    auto sut = std::make_unique<test_mem_pipe_endpoint>(true, m_thread_pool);
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
    event sync_event;
    auto data = create_test_data();
    auto pool = std::make_shared<thread_pool>(1);
    pool->post([&]() { sync_event.wait(); });
    auto buffers = std::make_shared<mem_buffers>(pool);
    MockFunction<void(pipe_op_res)> write_callback;
    EXPECT_CALL(write_callback, Call(pipe_op_res::timeout))
        .Times(1);
    test_mem_pipe_endpoint client_endpoint(false, pool);
    client_endpoint.set_buffers(buffers);

    auto f = client_endpoint.write_async(data.copy(), std::chrono::milliseconds(0))
        .then(write_callback.AsStdFunction());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event.set();
    f.get();
    pool->stop();
}

TEST_F(MemPipeEndpoint, WritesDataFromServerToClientTimeout)
{
    event sync_event;
    auto data = create_test_data();
    auto pool = std::make_shared<thread_pool>(1);
    pool->post([&]() { sync_event.wait(); });
    auto buffers = std::make_shared<mem_buffers>(pool);
    MockFunction<void(pipe_op_res)> write_callback;
    EXPECT_CALL(write_callback, Call(pipe_op_res::timeout))
        .Times(1);
    test_mem_pipe_endpoint server_endpoint(true, pool);
    server_endpoint.set_buffers(buffers);

    auto f = server_endpoint.write_async(data.copy(), std::chrono::milliseconds(0))
        .then(write_callback.AsStdFunction());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event.set();
    f.get();
    pool->stop();
}

TEST_F(MemPipeEndpoint, ReadDataFromClient)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(1);
    pool->post([&]() { sync_event.wait(); });
    auto buffers = std::make_shared<mem_buffers>(pool);
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::timeout, _))
        .Times(1);
    test_mem_pipe_endpoint server_endpoint(true, pool);
    server_endpoint.set_buffers(buffers);

    auto f = server_endpoint.read_async(std::chrono::milliseconds(0))
        .then(read_callback.AsStdFunction());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event.set();
    f.get();
    pool->stop();
}

TEST_F(MemPipeEndpoint, WritesDataFromServer)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(1);
    pool->post([&]() { sync_event.wait(); });
    auto buffers = std::make_shared<mem_buffers>(pool);
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::timeout, _))
        .Times(1);
    test_mem_pipe_endpoint client_endpoint(false, pool);
    client_endpoint.set_buffers(buffers);

    auto f = client_endpoint.read_async(std::chrono::milliseconds(0))
        .then(read_callback.AsStdFunction());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event.set();
    f.get();
    pool->stop();
}
