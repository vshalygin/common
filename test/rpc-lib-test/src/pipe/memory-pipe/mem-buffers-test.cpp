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

    buffer create_test_data()
    {
        buffer ans(2);
        ans[0] = std::byte(10);
        ans[1] = std::byte(6);
        return ans;
    }
}

class MemBuffers
    : public Test
{
protected:
    void SetUp() override
    {
        ON_CALL(m_invalidate_callback, Call)
            .WillByDefault([]() {});

        m_thread_pool = std::make_shared<thread_pool>(2);
    }

    void TearDown() override
    {
        m_thread_pool->stop();
    }

protected:
    MockFunction<void()> m_invalidate_callback;
    std::shared_ptr<thread_pool> m_thread_pool;
};

TEST_F(MemBuffers, IsValidAfterCreation)
{
    mem_buffers sut(m_thread_pool);

    ASSERT_TRUE(sut.is_valid());
}

TEST_F(MemBuffers, IsNotValidAfterInvalidation)
{
    mem_buffers sut(m_thread_pool);
    sut.invalidate();

    ASSERT_FALSE(sut.is_valid());
}

TEST_F(MemBuffers, WritesFromClientToServer)
{
    auto data = create_test_data();
    event sync_event;
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::success, BufferEq(data.data(), data.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    mem_buffers sut(m_thread_pool);
    sut.write_async_to_server(data.copy(), std::nullopt);
    sut.read_async_from_client(std::nullopt)
        .then(read_callback.AsStdFunction());

    sync_event.wait();
}

TEST_F(MemBuffers, WritesFromServerToClient)
{
    auto data = create_test_data();
    event sync_event;
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::success, BufferEq(data.data(), data.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    mem_buffers sut(m_thread_pool);
    sut.write_async_to_client(data.copy(), std::nullopt);
    sut.read_async_from_server(std::nullopt)
        .then(read_callback.AsStdFunction());

    sync_event.wait();
}

TEST_F(MemBuffers, SetFewInvalidateCallbacks)
{
    event sync_event;
    EXPECT_CALL(m_invalidate_callback, Call)
        .Times(2)
        .WillOnce(DoDefault())
        .WillOnce([&]() { sync_event.set(); });
    mem_buffers sut(m_thread_pool);
    sut.set_invalidate_callback(thread_pool_task(m_thread_pool.get(), m_invalidate_callback.AsStdFunction()));
    sut.set_invalidate_callback(thread_pool_task(m_thread_pool.get(), m_invalidate_callback.AsStdFunction()));
    EXPECT_EQ(sut.get_invalidate_callbacks_count(), 2);

    sut.invalidate();

    sync_event.wait();
    EXPECT_EQ(sut.get_invalidate_callbacks_count(), 0);
}

TEST_F(MemBuffers, SetExecuteInvalidateCallbacksImmediatelyIfInvalidated)
{
    event sync_event;
    EXPECT_CALL(m_invalidate_callback, Call)
        .Times(2)
        .WillOnce(DoDefault())
        .WillOnce([&]() { sync_event.set(); });
    mem_buffers sut(m_thread_pool);
    sut.invalidate();

    sut.set_invalidate_callback(thread_pool_task(m_thread_pool.get(), m_invalidate_callback.AsStdFunction()));
    sut.set_invalidate_callback(thread_pool_task(m_thread_pool.get(), m_invalidate_callback.AsStdFunction()));

    sync_event.wait();
    EXPECT_EQ(sut.get_invalidate_callbacks_count(), 0);
}

TEST_F(MemBuffers, ExecuteInvalidateCallbacksOnDestruction)
{
    event sync_event;
    EXPECT_CALL(m_invalidate_callback, Call)
        .Times(2)
        .WillOnce(DoDefault())
        .WillOnce([&]() { sync_event.set(); });
    auto sut = std::make_unique<mem_buffers>(m_thread_pool);
    sut->set_invalidate_callback(thread_pool_task(m_thread_pool.get(), m_invalidate_callback.AsStdFunction()));
    sut->set_invalidate_callback(thread_pool_task(m_thread_pool.get(), m_invalidate_callback.AsStdFunction()));

    sut.reset();
    sync_event.wait();
}

TEST_F(MemBuffers, WritesFromClientToServerTimeout)
{
    auto sync_event = std::make_shared<event>();
    auto pool = std::make_shared<thread_pool>(1);
    pool->post([sync_event]() { sync_event->wait(); });
    auto data = create_test_data();
    MockFunction<void(pipe_op_res)> write_callback;
    EXPECT_CALL(write_callback, Call(pipe_op_res::timeout))
        .Times(1);

    mem_buffers sut(pool);
    auto f = sut.write_async_to_server(data.copy(), std::chrono::milliseconds(0))
        .then(write_callback.AsStdFunction());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event->set();
    f.get();
    pool->stop();
}

TEST_F(MemBuffers, WritesFromServerToClientTimeout)
{
    auto sync_event = std::make_shared<event>();
    auto pool = std::make_shared<thread_pool>(1);
    pool->post([sync_event]() { sync_event->wait(); });
    auto data = create_test_data();
    MockFunction<void(pipe_op_res)> write_callback;
    EXPECT_CALL(write_callback, Call(pipe_op_res::timeout))
        .Times(1);

    mem_buffers sut(pool);
    auto f = sut.write_async_to_client(data.copy(), std::chrono::milliseconds(0))
        .then(write_callback.AsStdFunction());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event->set();
    f.get();
    pool->stop();
}

TEST_F(MemBuffers, ReadsFromClientTimeout)
{
    auto sync_event = std::make_shared<event>();
    auto pool = std::make_shared<thread_pool>(1);
    pool->post([sync_event]() { sync_event->wait(); });
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::timeout, _))
        .Times(1);

    mem_buffers sut(pool);
    auto f = sut.read_async_from_client(std::chrono::milliseconds(0))
        .then(read_callback.AsStdFunction());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event->set();
    f.get();
    pool->stop();
}

TEST_F(MemBuffers, ReadsFromServerTimeout)
{
    auto sync_event = std::make_shared<event>();
    auto pool = std::make_shared<thread_pool>(1);
    pool->post([sync_event]() { sync_event->wait(); });
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_op_res::timeout, _))
        .Times(1);

    mem_buffers sut(pool);
    auto f = sut.read_async_from_server(std::chrono::milliseconds(0))
        .then(read_callback.AsStdFunction());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event->set();
    f.get();
    pool->stop();
}
