#ifdef _WIN32
#include <rpc-lib/pipe/win-pipe/win-pipe-endpoint.h>
#include <rpc-lib/consts.h>

#include <common-lib/synchronization/event.h>
#include <common-lib/thread/thread.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

using namespace vshalygin::cl;
using namespace vshalygin::rpc;
using namespace vshalygin::rpc::internal;
using namespace vshalygin::win;
using namespace testing;

namespace {
    constexpr auto operation_timeout = std::chrono::seconds(5);
    constexpr auto immediate_timeout = std::chrono::milliseconds(0);

    struct pipe_result
    {
        pipe_wait_res state = pipe_wait_res::failed;
        pipe_handle pipe;
    };

    struct endpoint_read_result
    {
        pipe_op_res state = pipe_op_res::failed;
        buffer data;
    };

    struct endpoint_pair
    {
        std::unique_ptr<win_pipe_endpoint> server;
        std::unique_ptr<win_pipe_endpoint> client;
    };
}

class WinPipeEndpoint
    : public Test
{
protected:
    using create_operation = win_pipe_create_operation;
    using open_operation = win_pipe_open_operation;
    using pipe_future = future<thread_pool, ftuple<pipe_wait_res, pipe_handle>>;
    using read_future = ipipe_endpoint::read_future;
    using write_future = ipipe_endpoint::write_future;

    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(4);
        m_iocp_owner = win_pipe_iocp_owner::create();
    }

    void TearDown() override
    {
        m_iocp_owner.reset();
        m_thread_pool->stop();
    }

    static std::wstring make_pipe_name()
    {
        static std::atomic<std::uint64_t> next_id = 0;

        return L"rpc-lib-test-endpoint-"
            + std::to_wstring(::GetCurrentProcessId())
            + L"-"
            + std::to_wstring(next_id.fetch_add(1, std::memory_order_relaxed));
    }

    static buffer make_message(size_t size, size_t seed = 0)
    {
        buffer result(size);
        for(size_t i = 0; i < result.size(); ++i) {
            result[i] = static_cast<std::byte>((i * 47u + seed * 29u + 23u) % 251u);
        }
        return result;
    }

    template<typename Future, typename Cancel>
    static void wait_for_result(Future &future, Cancel &&cancel, const char *operation_name)
    {
        if(!future.wait_for(operation_timeout)) {
            ADD_FAILURE() << operation_name << " did not complete in time";
            cancel();

            // Keep the endpoint implementation and its OVERLAPPED operation alive
            // until invalidation has produced a terminal future value.
            future.wait();
        }
    }

    static pipe_result get_pipe_result(pipe_future &future)
    {
        pipe_result result;
        future.get().lock().with([&](pipe_wait_res state, pipe_handle &&pipe) {
            result.state = state;
            result.pipe = std::move(pipe);
        });
        return result;
    }

    static endpoint_read_result get_read_result(read_future &future)
    {
        endpoint_read_result result;
        future.get().lock().with([&](pipe_op_res state, buffer &&data) {
            result.state = state;
            result.data = std::move(data);
        });
        return result;
    }

    static pipe_op_res get_write_result(write_future &future)
    {
        auto result = pipe_op_res::failed;
        future.get().lock().with([&](pipe_op_res state) { result = state; });
        return result;
    }

    endpoint_pair create_endpoints()
    {
        auto pipe_name = make_pipe_name();
        auto create_op = create_operation::create(pipe_name, m_thread_pool.get());
        auto create_future = m_iocp_owner->create_pipe_async(create_op);

        open_operation open_op(pipe_name, m_thread_pool.get());
        auto open_future = m_iocp_owner->open_pipe_async(&open_op);

        auto create_ready = create_future.wait_for(operation_timeout);
        auto open_ready = open_future.wait_for(operation_timeout);
        if(!create_ready || !open_ready) {
            ADD_FAILURE() << "The endpoint handles were not connected in time";
            m_iocp_owner->cancel_create(create_op, false);
            open_op.cancel(false);
            create_future.wait();
            open_future.wait();
        }

        auto server = get_pipe_result(create_future);
        auto client = get_pipe_result(open_future);
        EXPECT_EQ(server.state, pipe_wait_res::success);
        EXPECT_EQ(client.state, pipe_wait_res::success);
        EXPECT_FALSE(server.pipe.empty());
        EXPECT_FALSE(client.pipe.empty());

        endpoint_pair result;
        if(!server.pipe.empty()) {
            result.server = std::make_unique<win_pipe_endpoint>(
                std::move(server.pipe), m_iocp_owner, m_thread_pool.get());
        }
        if(!client.pipe.empty()) {
            result.client = std::make_unique<win_pipe_endpoint>(
                std::move(client.pipe), m_iocp_owner, m_thread_pool.get());
        }
        return result;
    }

    static void wait_for_read(read_future &future,
                              win_pipe_endpoint &endpoint,
                              const char *operation_name)
    {
        wait_for_result(future, [&] { endpoint.invalidate(); }, operation_name);
    }

    static void wait_for_write(write_future &future,
                               win_pipe_endpoint &endpoint,
                               const char *operation_name)
    {
        wait_for_result(future, [&] { endpoint.invalidate(); }, operation_name);
    }

protected:
    std::shared_ptr<thread_pool> m_thread_pool;
    std::shared_ptr<win_pipe_iocp_owner> m_iocp_owner;
};

TEST_F(WinPipeEndpoint, IsConnectedAfterCreation)
{
    auto endpoints = create_endpoints();
    ASSERT_TRUE(endpoints.server);
    ASSERT_TRUE(endpoints.client);

    EXPECT_TRUE(endpoints.server->is_connected());
    EXPECT_TRUE(endpoints.client->is_connected());
}

TEST_F(WinPipeEndpoint, TransfersLargeMessage)
{
    auto endpoints = create_endpoints();
    ASSERT_TRUE(endpoints.server);
    ASSERT_TRUE(endpoints.client);
    auto message = make_message(8192u * 7u + 113u);
    auto expected = message.copy();

    auto read = endpoints.server->read_async();
    auto write = endpoints.client->write_async(std::move(message));
    wait_for_read(read, *endpoints.server, "The large endpoint read");
    wait_for_write(write, *endpoints.client, "The large endpoint write");

    auto read_result = get_read_result(read);
    EXPECT_EQ(get_write_result(write), pipe_op_res::success);
    EXPECT_EQ(read_result.state, pipe_op_res::success);
    EXPECT_EQ(read_result.data, expected);
}

TEST_F(WinPipeEndpoint, PreservesOrderOfQueuedReadsAndWrites)
{
    auto endpoints = create_endpoints();
    ASSERT_TRUE(endpoints.server);
    ASSERT_TRUE(endpoints.client);
    auto first = make_message(101, 1);
    auto second = make_message(203, 2);
    auto third = make_message(307, 3);
    auto expected_first = first.copy();
    auto expected_second = second.copy();
    auto expected_third = third.copy();

    auto first_read = endpoints.server->read_async();
    auto second_read = endpoints.server->read_async();
    auto third_read = endpoints.server->read_async();
    auto first_write = endpoints.client->write_async(std::move(first));
    auto second_write = endpoints.client->write_async(std::move(second));
    auto third_write = endpoints.client->write_async(std::move(third));

    wait_for_read(first_read, *endpoints.server, "The first queued read");
    wait_for_read(second_read, *endpoints.server, "The second queued read");
    wait_for_read(third_read, *endpoints.server, "The third queued read");
    wait_for_write(first_write, *endpoints.client, "The first queued write");
    wait_for_write(second_write, *endpoints.client, "The second queued write");
    wait_for_write(third_write, *endpoints.client, "The third queued write");

    auto first_result = get_read_result(first_read);
    auto second_result = get_read_result(second_read);
    auto third_result = get_read_result(third_read);
    EXPECT_EQ(get_write_result(first_write), pipe_op_res::success);
    EXPECT_EQ(get_write_result(second_write), pipe_op_res::success);
    EXPECT_EQ(get_write_result(third_write), pipe_op_res::success);
    EXPECT_EQ(first_result.state, pipe_op_res::success);
    EXPECT_EQ(second_result.state, pipe_op_res::success);
    EXPECT_EQ(third_result.state, pipe_op_res::success);
    EXPECT_EQ(first_result.data, expected_first);
    EXPECT_EQ(second_result.data, expected_second);
    EXPECT_EQ(third_result.data, expected_third);
}

TEST_F(WinPipeEndpoint, ReadTimeoutDoesNotDisconnectEndpoint)
{
    auto endpoints = create_endpoints();
    ASSERT_TRUE(endpoints.server);

    auto read = endpoints.server->read_async(immediate_timeout);
    wait_for_read(read, *endpoints.server, "The timed out endpoint read");

    auto result = get_read_result(read);
    EXPECT_EQ(result.state, pipe_op_res::timeout);
    EXPECT_EQ(result.data.size(), 0u);
    EXPECT_TRUE(endpoints.server->is_connected());
}

TEST_F(WinPipeEndpoint, WriteTimeoutDoesNotDisconnectEndpoint)
{
    auto endpoints = create_endpoints();
    ASSERT_TRUE(endpoints.client);
    auto message = make_message(MaxTransferMessageSize);

    auto write = endpoints.client->write_async(std::move(message), immediate_timeout);
    wait_for_write(write, *endpoints.client, "The timed out endpoint write");

    EXPECT_EQ(get_write_result(write), pipe_op_res::timeout);
    EXPECT_TRUE(endpoints.client->is_connected());
}

TEST_F(WinPipeEndpoint, QueuedReadCanTimeOutBeforeActiveRead)
{
    auto endpoints = create_endpoints();
    ASSERT_TRUE(endpoints.server);

    auto active_read = endpoints.server->read_async();
    auto queued_read = endpoints.server->read_async(immediate_timeout);
    wait_for_read(queued_read, *endpoints.server, "The queued timed out read");

    auto queued_result = get_read_result(queued_read);
    EXPECT_EQ(queued_result.state, pipe_op_res::timeout);
    EXPECT_EQ(queued_result.data.size(), 0u);
    EXPECT_FALSE(active_read.wait_for(immediate_timeout));
    EXPECT_TRUE(endpoints.server->is_connected());

    endpoints.server->invalidate();
    wait_for_read(active_read, *endpoints.server, "The active read during cleanup");
    EXPECT_EQ(get_read_result(active_read).state, pipe_op_res::canceled);
}

TEST_F(WinPipeEndpoint, QueuedWriteCanTimeOutBeforeActiveWrite)
{
    auto endpoints = create_endpoints();
    ASSERT_TRUE(endpoints.client);
    auto large_message = make_message(MaxTransferMessageSize);
    auto queued_message = make_message(31);

    auto active_write = endpoints.client->write_async(std::move(large_message));
    auto queued_write = endpoints.client->write_async(std::move(queued_message), immediate_timeout);
    wait_for_write(queued_write, *endpoints.client, "The queued timed out write");

    EXPECT_EQ(get_write_result(queued_write), pipe_op_res::timeout);
    EXPECT_FALSE(active_write.wait_for(immediate_timeout));
    EXPECT_TRUE(endpoints.client->is_connected());

    endpoints.client->invalidate();
    wait_for_write(active_write, *endpoints.client, "The active write during cleanup");
    EXPECT_EQ(get_write_result(active_write), pipe_op_res::canceled);
}

TEST_F(WinPipeEndpoint, InvalidateCancelsPendingOperationsAndIsIdempotent)
{
    auto endpoints = create_endpoints();
    ASSERT_TRUE(endpoints.server);
    auto callback_event = std::make_shared<event>();
    auto callback_count = std::make_shared<std::atomic<unsigned>>(0);
    endpoints.server->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(),
        [callback_event, callback_count] {
            callback_count->fetch_add(1, std::memory_order_relaxed);
            callback_event->set();
        }));

    auto read = endpoints.server->read_async();
    auto message = make_message(MaxTransferMessageSize);
    auto write = endpoints.server->write_async(std::move(message));

    endpoints.server->invalidate();
    wait_for_read(read, *endpoints.server, "The invalidated endpoint read");
    wait_for_write(write, *endpoints.server, "The invalidated endpoint write");

    EXPECT_EQ(get_read_result(read).state, pipe_op_res::canceled);
    EXPECT_EQ(get_write_result(write), pipe_op_res::canceled);
    EXPECT_FALSE(endpoints.server->is_connected());
    EXPECT_TRUE(callback_event->wait_for(operation_timeout));
    EXPECT_EQ(callback_count->load(std::memory_order_relaxed), 1u);

    endpoints.server->invalidate();
    EXPECT_EQ(callback_count->load(std::memory_order_relaxed), 1u);

    auto failed_read = endpoints.server->read_async();
    auto failed_write = endpoints.server->write_async(make_message(1));
    auto failed_read_result = get_read_result(failed_read);
    EXPECT_EQ(failed_read_result.state, pipe_op_res::failed);
    EXPECT_EQ(failed_read_result.data.size(), 0u);
    EXPECT_EQ(get_write_result(failed_write), pipe_op_res::failed);
}

TEST_F(WinPipeEndpoint, DestructorCancelsPendingOperations)
{
    auto endpoints = create_endpoints();
    ASSERT_TRUE(endpoints.server);

    auto read = endpoints.server->read_async();
    auto message = make_message(MaxTransferMessageSize);
    auto write = endpoints.server->write_async(std::move(message));
    endpoints.server.reset();

    wait_for_result(read, [] {}, "The read canceled by endpoint destruction");
    wait_for_result(write, [] {}, "The write canceled by endpoint destruction");
    EXPECT_EQ(get_read_result(read).state, pipe_op_res::canceled);
    EXPECT_EQ(get_write_result(write), pipe_op_res::canceled);
}

TEST_F(WinPipeEndpoint, PeerDisconnectFailsPendingReadsAndInvokesCallback)
{
    auto endpoints = create_endpoints();
    ASSERT_TRUE(endpoints.server);
    ASSERT_TRUE(endpoints.client);
    auto callback_event = std::make_shared<event>();
    endpoints.server->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(), [callback_event] { callback_event->set(); }));

    auto first_read = endpoints.server->read_async();
    auto second_read = endpoints.server->read_async();
    endpoints.client->invalidate();

    wait_for_read(first_read, *endpoints.server, "The first read after peer disconnect");
    wait_for_read(second_read, *endpoints.server, "The second read after peer disconnect");
    EXPECT_EQ(get_read_result(first_read).state, pipe_op_res::failed);
    EXPECT_EQ(get_read_result(second_read).state, pipe_op_res::failed);
    EXPECT_FALSE(endpoints.server->is_connected());
    EXPECT_TRUE(callback_event->wait_for(operation_timeout));

    auto failed_read = endpoints.server->read_async();
    EXPECT_EQ(get_read_result(failed_read).state, pipe_op_res::failed);
}

TEST_F(WinPipeEndpoint, PeerDisconnectFailsPendingWriteAndInvokesCallback)
{
    auto endpoints = create_endpoints();
    ASSERT_TRUE(endpoints.server);
    ASSERT_TRUE(endpoints.client);
    auto callback_event = std::make_shared<event>();
    endpoints.server->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(), [callback_event] { callback_event->set(); }));
    auto message = make_message(MaxTransferMessageSize);

    auto write = endpoints.server->write_async(std::move(message));
    endpoints.client->invalidate();

    wait_for_write(write, *endpoints.server, "The write after peer disconnect");
    EXPECT_EQ(get_write_result(write), pipe_op_res::failed);
    EXPECT_FALSE(endpoints.server->is_connected());
    EXPECT_TRUE(callback_event->wait_for(operation_timeout));
}

TEST_F(WinPipeEndpoint, DisconnectCallbackSetAfterInvalidationExecutesImmediately)
{
    auto endpoints = create_endpoints();
    ASSERT_TRUE(endpoints.server);
    auto callback_event = std::make_shared<event>();

    endpoints.server->invalidate();
    endpoints.server->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(), [callback_event] { callback_event->set(); }));

    EXPECT_TRUE(callback_event->wait_for(operation_timeout));
}

TEST_F(WinPipeEndpoint, InvokesEveryDisconnectCallbackOnlyOnce)
{
    auto endpoints = create_endpoints();
    ASSERT_TRUE(endpoints.server);
    auto first_event = std::make_shared<event>();
    auto second_event = std::make_shared<event>();
    auto callback_count = std::make_shared<std::atomic<unsigned>>(0);
    endpoints.server->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(),
        [first_event, callback_count] {
            callback_count->fetch_add(1, std::memory_order_relaxed);
            first_event->set();
        }));
    endpoints.server->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(),
        [second_event, callback_count] {
            callback_count->fetch_add(1, std::memory_order_relaxed);
            second_event->set();
        }));

    endpoints.server->invalidate();
    EXPECT_TRUE(first_event->wait_for(operation_timeout));
    EXPECT_TRUE(second_event->wait_for(operation_timeout));
    EXPECT_EQ(callback_count->load(std::memory_order_relaxed), 2u);

    endpoints.server->invalidate();
    EXPECT_EQ(callback_count->load(std::memory_order_relaxed), 2u);
}
#endif
