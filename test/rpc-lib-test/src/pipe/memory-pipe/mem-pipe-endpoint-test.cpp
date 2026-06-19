#include <rpc-lib/pipe/memory-pipe/mem-pipe-endpoint.h>
#include <rpc-lib/pipe/memory-pipe/mem-buffers.h>
#include <common-lib/syncronization/event/event.h>

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

    void dummy_write_callback(pipe_op_res)
    {}

    void dummy_read_callback(pipe_op_res, buffer &&)
    {}
}

TEST(MemPipeEndpoint, InNotConnectedAfterCreation)
{
    auto pool = std::make_shared<thread_pool>(2);
    test_mem_pipe_endpoint sut(true, pool);

    ASSERT_FALSE(sut.is_connected());
}

TEST(MemPipeEndpoint, InConnectedAfterSettingBuffers)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);

    test_mem_pipe_endpoint sut(true, pool);
    sut.set_buffers(buffers);

    ASSERT_TRUE(sut.is_connected());
}

TEST(MemPipeEndpoint, InNotConnectedAfterUnderlyingBuffersInvalidated)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);

    test_mem_pipe_endpoint sut(true, pool);
    sut.set_buffers(buffers);
    buffers->invalidate();

    ASSERT_FALSE(sut.is_connected());
}

TEST(MemPipeEndpoint, SetsInvalidateCallbackAfterConnect)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    test_mem_pipe_endpoint sut(true, pool);
    sut.set_buffers(buffers);
    sut.set_disconnect_callback(callback.AsStdFunction());
    sut.invalidate();

    ASSERT_FALSE(sut.is_connected());
    sync_event.wait();
}

TEST(MemPipeEndpoint, SetsInvalidateCallbackBeforeConnect)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    test_mem_pipe_endpoint sut(true, pool);
    sut.set_disconnect_callback(callback.AsStdFunction());
    sut.set_buffers(buffers);
    sut.invalidate();

    ASSERT_FALSE(sut.is_connected());
    sync_event.wait();
}

TEST(MemPipeEndpoint, InvalidatesSettingBuffersIfWasInvalidatedBefore)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);

    test_mem_pipe_endpoint sut(true, pool);
    sut.invalidate();
    sut.set_buffers(buffers);

    ASSERT_FALSE(buffers->is_valid());
}

TEST(MemPipeEndpoint, InvalidatesUnderlyingBuffers)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);

    test_mem_pipe_endpoint sut(true, pool);
    sut.set_buffers(buffers);
    sut.invalidate();

    ASSERT_FALSE(buffers->is_valid());
}

TEST(MemPipeEndpoint, ExecuteDisconnectCallbackOnlyOnceIfBuffersWereNotSet)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    test_mem_pipe_endpoint sut(true, pool);
    sut.set_disconnect_callback(callback.AsStdFunction());
    sut.invalidate();
    sut.invalidate();

    sync_event.wait();
}

TEST(MemPipeEndpoint, WaitsConnect)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);

    test_mem_pipe_endpoint sut(true, pool);
    pool->post([&]() {
        auto res = sut.wait_connect();
        EXPECT_EQ(res, pipe_wait_res::connected);
        sync_event.set();
    });

    sut.set_buffers(buffers);

    sync_event.wait();
}

TEST(MemPipeEndpoint, StopWaitingConnectIfItIsAlreadyConnected)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);

    test_mem_pipe_endpoint sut(true, pool);
    sut.set_buffers(buffers);

    EXPECT_EQ(sut.wait_connect(), pipe_wait_res::connected);
}

TEST(MemPipeEndpoint, StopsWaitingConnectIfInvalidated)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);

    test_mem_pipe_endpoint sut(true, pool);
    pool->post([&]() {
        auto res = sut.wait_connect();
        EXPECT_EQ(res, pipe_wait_res::invalidated);
        sync_event.set();
    });

    sut.invalidate();

    sync_event.wait();
}

TEST(MemPipeEndpoint, StopsWaitingConnectIfItIsInvalidated)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);

    test_mem_pipe_endpoint sut(true, pool);
    sut.invalidate();

    EXPECT_EQ(sut.wait_connect(), pipe_wait_res::invalidated);
}

TEST(MemPipeEndpoint, WaitsConnectSuccessfullyBeforeTimeout)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);

    test_mem_pipe_endpoint sut(true, pool);
    pool->post([&]() {
        auto res = sut.wait_connect_for(std::chrono::seconds(100));
        EXPECT_EQ(res, pipe_wait_res::connected);
        sync_event.set();
    });

    sut.set_buffers(buffers);

    sync_event.wait();
}

TEST(MemPipeEndpoint, CancelsWaitingOnTimeout)
{
    auto pool = std::make_shared<thread_pool>(2);

    test_mem_pipe_endpoint sut(true, pool);

    auto res = sut.wait_connect_for(std::chrono::microseconds(1));
    EXPECT_EQ(res, pipe_wait_res::timeout);
}

TEST(MemPipeEndpoint, StopWaitingConnectUntilTimeoutIfItIsAlreadyConnected)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);

    test_mem_pipe_endpoint sut(true, pool);
    sut.set_buffers(buffers);

    EXPECT_EQ(sut.wait_connect_for(std::chrono::seconds(100)), pipe_wait_res::connected);
}

TEST(MemPipeEndpoint, StopsWaitingConnectUntilTimeoutIfInvalidated)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);

    test_mem_pipe_endpoint sut(true, pool);
    pool->post([&]() {
        auto res = sut.wait_connect_for(std::chrono::seconds(100));
        EXPECT_EQ(res, pipe_wait_res::invalidated);
        sync_event.set();
    });

    sut.invalidate();
    sync_event.wait();
}

TEST(MemPipeEndpoint, StopsWaitingConnectUntilTimeoutIfItIsInvalidated)
{
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);

    test_mem_pipe_endpoint sut(true, pool);
    sut.invalidate();

    EXPECT_EQ(sut.wait_connect_for(std::chrono::seconds(100)), pipe_wait_res::invalidated);
}

TEST(MemPipeEndpoint, ExecuteWriteCallbackWithFailedCodeIfBuffersWereNotSet)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);

    MockFunction<void(pipe_op_res)> callback;
    EXPECT_CALL(callback, Call(pipe_op_res::failed))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    test_mem_pipe_endpoint sut(true, pool);
    sut.write_async({}, callback.AsStdFunction());

    sync_event.wait();
}

TEST(MemPipeEndpoint, WritesDataFromClientToServer)
{
    event sync_event;
    auto data = create_test_data();
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::success, BufferEq(data.data(), data.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    test_mem_pipe_endpoint server_endpoint(true, pool);
    test_mem_pipe_endpoint client_endpoint(false, pool);
    server_endpoint.set_buffers(buffers);
    client_endpoint.set_buffers(buffers);

    client_endpoint.write_async(data.copy(), &dummy_write_callback);
    server_endpoint.read_async(read_callback.AsStdFunction());

    sync_event.wait();
}

TEST(MemPipeEndpoint, WritesDataFromServerToClient)
{
    event sync_event;
    auto data = create_test_data();
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::success, BufferEq(data.data(), data.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    test_mem_pipe_endpoint server_endpoint(true, pool);
    test_mem_pipe_endpoint client_endpoint(false, pool);
    server_endpoint.set_buffers(buffers);
    client_endpoint.set_buffers(buffers);

    server_endpoint.write_async(data.copy(), &dummy_write_callback);
    client_endpoint.read_async(read_callback.AsStdFunction());

    sync_event.wait();
}

TEST(MemPipeEndpoint, FailsSyncWriteIfNoBuffersSet)
{
    auto pool = std::make_shared<thread_pool>(2);
    test_mem_pipe_endpoint sut(true, pool);

    auto r = sut.try_to_write_for({}, std::chrono::hours(1));

    ASSERT_FALSE(r);
}

TEST(MemPipeEndpoint, WritesAndReadDataSynchronously)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto buffers = std::make_shared<mem_buffers>(pool);
    auto data = create_test_data();
    test_mem_pipe_endpoint server_enpoint(true, pool);
    test_mem_pipe_endpoint client_enpoint(false, pool);
    server_enpoint.set_buffers(buffers);
    client_enpoint.set_buffers(buffers);

    pool->post([&]() {
        auto r = server_enpoint.try_to_write_for(data.copy(), std::chrono::hours(1));
        EXPECT_TRUE(r);
        sync_event.set();
    });
    auto res = client_enpoint.try_to_read_for(std::chrono::hours(1));

    sync_event.wait();
    ASSERT_TRUE(res && *res == data);
}

TEST(MemPipeEndpoint, FailsSyncReadIfNoBuffersSet)
{
    auto pool = std::make_shared<thread_pool>(2);
    test_mem_pipe_endpoint sut(true, pool);

    auto r = sut.try_to_read_for(std::chrono::hours(1));

    ASSERT_FALSE(r.has_value());
}

TEST(MemPipeEndpoint, ExecuteDisconnectCallbackOnDestruction)
{
    event sync_event;
    auto pool = std::make_shared<thread_pool>(2);
    auto sut = std::make_unique<test_mem_pipe_endpoint>(true, pool);
    MockFunction<void()> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    sut->set_disconnect_callback(callback.AsStdFunction());

    sut.reset();

    sync_event.wait();
    Mock::VerifyAndClearExpectations(&callback);
}
