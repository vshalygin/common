#ifdef _WIN32
#include <rpc-lib/pipe/win-pipe/win-pipe-iocp-owner.h>
#include <rpc-lib/consts.h>

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

    struct read_result
    {
        win_pipe_operation_res state = win_pipe_operation_res::unknown;
        buffer data;
    };

    struct connected_pipe
    {
        std::shared_ptr<value_locker<pipe_handle>> server;
        std::shared_ptr<value_locker<pipe_handle>> client;
    };
}

class WinPipeIocpOwner
    : public Test
{
protected:
    using owner = win_pipe_iocp_owner;
    using create_operation = win_pipe_create_operation;
    using open_operation = win_pipe_open_operation;
    using read_operation = win_pipe_read_operation;
    using write_operation = win_pipe_write_operation;
    using pipe_future = vshalygin::rpc::future<ftuple<pipe_wait_res, pipe_handle>>;
    using read_future = vshalygin::rpc::future<ftuple<win_pipe_operation_res, buffer>>;
    using write_future = vshalygin::rpc::future<win_pipe_operation_res>;

    void SetUp() override
    {
        m_thread_pool = std::make_unique<thread_pool>(2);
        m_owner = owner::create();
    }

    void TearDown() override
    {
        m_owner.reset();
        m_thread_pool->stop();
    }

    static std::wstring make_pipe_name()
    {
        static std::atomic<std::uint64_t> next_id = 0;

        return L"rpc-lib-test-iocp-owner-"
            + std::to_wstring(::GetCurrentProcessId())
            + L"-"
            + std::to_wstring(next_id.fetch_add(1, std::memory_order_relaxed));
    }

    static buffer make_message(size_t size)
    {
        buffer result(size);
        for(size_t i = 0; i < result.size(); ++i) {
            result[i] = static_cast<std::byte>((i * 43u + 19u) % 251u);
        }
        return result;
    }

    static std::shared_ptr<value_locker<pipe_handle>> make_empty_pipe()
    {
        return std::make_shared<value_locker<pipe_handle>>();
    }

    template<typename Future, typename Cancel>
    static void wait_for_result(Future &future, Cancel &&cancel, const char *operation_name)
    {
        if(!future.wait_for(operation_timeout)) {
            ADD_FAILURE() << operation_name << " did not complete in time";
            cancel();

            // Keep all operation and OVERLAPPED objects alive until cancellation has
            // produced the terminal result.
            future.wait();
        }
    }

    static pipe_result get_pipe_result(pipe_future &future)
    {
        pipe_result result;
        future.get().apply([&](pipe_wait_res state, pipe_handle &&pipe) {
            result.state = state;
            result.pipe = std::move(pipe);
        });
        return result;
    }

    static read_result get_read_result(read_future &future)
    {
        read_result result;
        future.get().apply([&](win_pipe_operation_res state, buffer &&data) {
            result.state = state;
            result.data = std::move(data);
        });
        return result;
    }

    static win_pipe_operation_res get_write_result(write_future &future)
    {
        auto result = win_pipe_operation_res::unknown;
        future.get().apply([&](win_pipe_operation_res state) { result = state; });
        return result;
    }

    connected_pipe create_connected_pipe()
    {
        auto pipe_name = make_pipe_name();
        auto create_op = create_operation::create(pipe_name, m_thread_pool.get());
        auto create_future = m_owner->create_pipe_async(create_op);

        open_operation open_op(pipe_name, m_thread_pool.get());
        auto open_future = m_owner->open_pipe_async(&open_op);

        auto create_ready = create_future.wait_for(operation_timeout);
        auto open_ready = open_future.wait_for(operation_timeout);
        if(!create_ready || !open_ready) {
            ADD_FAILURE() << "The owner did not create and open a connected pipe in time";
            m_owner->cancel_create(create_op, false);
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

        connected_pipe result;
        if(!server.pipe.empty()) {
            result.server = std::make_shared<value_locker<pipe_handle>>(std::move(server.pipe));
        }
        if(!client.pipe.empty()) {
            result.client = std::make_shared<value_locker<pipe_handle>>(std::move(client.pipe));
        }
        return result;
    }

    void wait_for_read_queue_barrier()
    {
        auto op = read_operation::create(make_empty_pipe(), m_thread_pool.get());
        op->set_canceled_if_possible();
        auto future = op->get_future();
        m_owner->read_async(op);

        wait_for_result(future,
                        [] {},
                        "The read IO thread barrier");
        auto result = get_read_result(future);
        EXPECT_EQ(result.state, win_pipe_operation_res::canceled);
        EXPECT_EQ(result.data.size(), 0u);
    }

    void wait_for_write_queue_barrier()
    {
        auto op = write_operation::create(make_empty_pipe(), buffer{}, m_thread_pool.get());
        op->set_canceled_if_possible();
        auto future = op->get_future();
        m_owner->write_async(op);

        wait_for_result(future,
                        [] {},
                        "The write IO thread barrier");
        EXPECT_EQ(get_write_result(future), win_pipe_operation_res::canceled);
    }

    static void close_pipe(const std::shared_ptr<value_locker<pipe_handle>> &pipe)
    {
        pipe->lock()->reset();
    }

protected:
    std::unique_ptr<thread_pool> m_thread_pool;
    std::shared_ptr<owner> m_owner;
};

TEST_F(WinPipeIocpOwner, CreatesAndOpensPipeThenTransfersLargeMessage)
{
    auto pipe = create_connected_pipe();
    ASSERT_TRUE(pipe.server);
    ASSERT_TRUE(pipe.client);

    auto read_op = read_operation::create(pipe.server, m_thread_pool.get());
    auto read_future = read_op->get_future();
    auto message = make_message(8192u * 7u + 113u);
    auto expected = message.copy();
    auto write_op = write_operation::create(pipe.client, std::move(message), m_thread_pool.get());
    auto write_future = write_op->get_future();

    m_owner->read_async(read_op);
    m_owner->write_async(write_op);

    wait_for_result(read_future,
                    [&] {
                        read_op->set_canceled_if_possible();
                        m_owner->cancel_read(read_op);
                    },
                    "The large read operation");
    wait_for_result(write_future,
                    [&] {
                        write_op->set_canceled_if_possible();
                        m_owner->cancel_write(write_op);
                    },
                    "The large write operation");

    auto read = get_read_result(read_future);
    auto write = get_write_result(write_future);
    EXPECT_EQ(write, win_pipe_operation_res::success);
    EXPECT_EQ(read.state, win_pipe_operation_res::success);
    EXPECT_EQ(read.data, expected);
}

TEST_F(WinPipeIocpOwner, CancelsPendingCreateOnItsIssuingThread)
{
    auto op = create_operation::create(make_pipe_name(), m_thread_pool.get());
    auto future = m_owner->create_pipe_async(op);

    m_owner->cancel_create(op, false);
    wait_for_result(future,
                    [&] { m_owner->cancel_create(op, false); },
                    "The canceled create operation");

    auto result = get_pipe_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::canceled);
    EXPECT_TRUE(result.pipe.empty());
}

TEST_F(WinPipeIocpOwner, TimesOutPendingCreateOnItsIssuingThread)
{
    auto op = create_operation::create(make_pipe_name(), m_thread_pool.get());
    auto future = m_owner->create_pipe_async(op);

    m_owner->cancel_create(op, true);
    wait_for_result(future,
                    [&] { m_owner->cancel_create(op, true); },
                    "The timed out create operation");

    auto result = get_pipe_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::timeout);
    EXPECT_TRUE(result.pipe.empty());
}

TEST_F(WinPipeIocpOwner, MapsPipeCreationFailure)
{
    auto op = create_operation::create(std::wstring(512, L'x'), m_thread_pool.get());
    auto future = m_owner->create_pipe_async(op);

    wait_for_result(future,
                    [&] { m_owner->cancel_create(op, false); },
                    "The failed create operation");

    auto result = get_pipe_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::failed);
    EXPECT_TRUE(result.pipe.empty());
}

TEST_F(WinPipeIocpOwner, ResolvesAlreadyCanceledReadAndWriteWithoutStartingIo)
{
    wait_for_read_queue_barrier();
    wait_for_write_queue_barrier();
}

TEST_F(WinPipeIocpOwner, MapsImmediateReadStartFailure)
{
    auto op = read_operation::create(make_empty_pipe(), m_thread_pool.get());
    auto future = op->get_future();

    m_owner->read_async(op);
    wait_for_result(future,
                    [&] {
                        op->set_canceled_if_possible();
                        m_owner->cancel_read(op);
                    },
                    "The failed read operation");

    auto result = get_read_result(future);
    EXPECT_EQ(result.state, win_pipe_operation_res::failed);
    EXPECT_EQ(result.data.size(), 0u);
}

TEST_F(WinPipeIocpOwner, MapsImmediateWriteStartFailure)
{
    auto op = write_operation::create(make_empty_pipe(), buffer{}, m_thread_pool.get());
    auto future = op->get_future();

    m_owner->write_async(op);
    wait_for_result(future,
                    [&] {
                        op->set_canceled_if_possible();
                        m_owner->cancel_write(op);
                    },
                    "The failed write operation");

    EXPECT_EQ(get_write_result(future), win_pipe_operation_res::failed);
}

TEST_F(WinPipeIocpOwner, CancelsPendingRead)
{
    auto pipe = create_connected_pipe();
    ASSERT_TRUE(pipe.server);
    auto op = read_operation::create(pipe.server, m_thread_pool.get());
    auto future = op->get_future();

    m_owner->read_async(op);
    wait_for_read_queue_barrier();
    ASSERT_FALSE(future.wait_for(immediate_timeout));

    op->set_canceled_if_possible();
    m_owner->cancel_read(op);
    wait_for_result(future,
                    [&] { m_owner->cancel_read(op); },
                    "The canceled read operation");

    auto result = get_read_result(future);
    EXPECT_EQ(result.state, win_pipe_operation_res::canceled);
    EXPECT_EQ(result.data.size(), 0u);
}

TEST_F(WinPipeIocpOwner, TimesOutPendingRead)
{
    auto pipe = create_connected_pipe();
    ASSERT_TRUE(pipe.server);
    auto op = read_operation::create(pipe.server, m_thread_pool.get());
    auto future = op->get_future();

    m_owner->read_async(op);
    wait_for_read_queue_barrier();
    ASSERT_FALSE(future.wait_for(immediate_timeout));

    op->set_timeout_if_possible();
    m_owner->cancel_read(op);
    wait_for_result(future,
                    [&] { m_owner->cancel_read(op); },
                    "The timed out read operation");

    auto result = get_read_result(future);
    EXPECT_EQ(result.state, win_pipe_operation_res::timeout);
    EXPECT_EQ(result.data.size(), 0u);
}

TEST_F(WinPipeIocpOwner, CancelsPendingWrite)
{
    auto pipe = create_connected_pipe();
    ASSERT_TRUE(pipe.server);
    auto message = make_message(MaxTransferMessageSize);
    auto op = write_operation::create(pipe.server, std::move(message), m_thread_pool.get());
    auto future = op->get_future();

    m_owner->write_async(op);
    wait_for_write_queue_barrier();
    ASSERT_FALSE(future.wait_for(immediate_timeout));

    op->set_canceled_if_possible();
    m_owner->cancel_write(op);
    wait_for_result(future,
                    [&] { m_owner->cancel_write(op); },
                    "The canceled write operation");

    EXPECT_EQ(get_write_result(future), win_pipe_operation_res::canceled);
}

TEST_F(WinPipeIocpOwner, TimesOutPendingWrite)
{
    auto pipe = create_connected_pipe();
    ASSERT_TRUE(pipe.server);
    auto message = make_message(MaxTransferMessageSize);
    auto op = write_operation::create(pipe.server, std::move(message), m_thread_pool.get());
    auto future = op->get_future();

    m_owner->write_async(op);
    wait_for_write_queue_barrier();
    ASSERT_FALSE(future.wait_for(immediate_timeout));

    op->set_timeout_if_possible();
    m_owner->cancel_write(op);
    wait_for_result(future,
                    [&] { m_owner->cancel_write(op); },
                    "The timed out write operation");

    EXPECT_EQ(get_write_result(future), win_pipe_operation_res::timeout);
}

TEST_F(WinPipeIocpOwner, PeerDisconnectFailsPendingRead)
{
    auto pipe = create_connected_pipe();
    ASSERT_TRUE(pipe.server);
    ASSERT_TRUE(pipe.client);
    auto op = read_operation::create(pipe.server, m_thread_pool.get());
    auto future = op->get_future();

    m_owner->read_async(op);
    wait_for_read_queue_barrier();
    ASSERT_FALSE(future.wait_for(immediate_timeout));

    close_pipe(pipe.client);
    wait_for_result(future,
                    [&] {
                        op->set_canceled_if_possible();
                        m_owner->cancel_read(op);
                    },
                    "The disconnected read operation");

    auto result = get_read_result(future);
    EXPECT_EQ(result.state, win_pipe_operation_res::failed);
    EXPECT_EQ(result.data.size(), 0u);
}

TEST_F(WinPipeIocpOwner, PeerDisconnectFailsPendingWrite)
{
    auto pipe = create_connected_pipe();
    ASSERT_TRUE(pipe.server);
    ASSERT_TRUE(pipe.client);
    auto message = make_message(MaxTransferMessageSize);
    auto op = write_operation::create(pipe.server, std::move(message), m_thread_pool.get());
    auto future = op->get_future();

    m_owner->write_async(op);
    wait_for_write_queue_barrier();
    ASSERT_FALSE(future.wait_for(immediate_timeout));

    close_pipe(pipe.client);
    wait_for_result(future,
                    [&] {
                        op->set_canceled_if_possible();
                        m_owner->cancel_write(op);
                    },
                    "The disconnected write operation");

    EXPECT_EQ(get_write_result(future), win_pipe_operation_res::failed);
}
#endif
