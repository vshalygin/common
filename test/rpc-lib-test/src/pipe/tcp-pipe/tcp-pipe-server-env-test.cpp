#include <rpc-lib/pipe/tcp-pipe/tcp-pipe-server-env.h>
#include <rpc-lib/pipe/tcp-pipe/tcp-pipe-client-env.h>
#include <rpc-lib/pipe/ipipe-endpoint.h>

#include <common-lib/thread/thread-pool/thread-pool.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace vshalygin::cl;
using namespace vshalygin::rpc;
using namespace testing;

namespace {
    using tcp = boost::asio::ip::tcp;

    constexpr auto operation_timeout = std::chrono::seconds(5);
    constexpr auto long_create_timeout = std::chrono::seconds(30);
    constexpr auto pending_create_timeout = std::chrono::milliseconds(100);
    constexpr auto pending_observation_timeout = std::chrono::milliseconds(100);
    constexpr size_t cancellation_race_iteration_count = 32;

    constexpr auto loopback_ip = "127.0.0.1";
    const auto loopback_address = boost::asio::ip::make_address_v4(loopback_ip);

    struct endpoint_result
    {
        pipe_wait_res state = pipe_wait_res::failed;
        std::shared_ptr<ipipe_endpoint> endpoint;
    };

    struct endpoint_read_result
    {
        pipe_op_res state = pipe_op_res::failed;
        buffer data;
    };
}

class TcpPipeServerEnv
    : public Test
{
protected:
    using server_env = tcp_pipe_server_env;
    using client_env = tcp_pipe_client_env;
    using endpoint_future = iserver_pipe_env::pipe_endpoint_future;
    using read_future = ipipe_endpoint::read_future;
    using write_future = ipipe_endpoint::write_future;

    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(4);
    }

    void TearDown() override
    {
        m_thread_pool->stop();
    }

    template<typename Future, typename Cancel>
    static bool wait_for_result(Future &future, Cancel &&cancel, const char *operation_name)
    {
        if(future.wait_for(operation_timeout)) {
            return true;
        }

        ADD_FAILURE() << operation_name << " did not complete in time";
        cancel();
        if(!future.wait_for(operation_timeout)) {
            ADD_FAILURE() << operation_name << " did not complete after cancellation";
            return false;
        }
        return true;
    }

    static endpoint_result get_endpoint_result(endpoint_future &future)
    {
        endpoint_result result;
        future.get().lock().with([&](pipe_wait_res state, std::shared_ptr<ipipe_endpoint> endpoint) {
            result.state = state;
            result.endpoint = std::move(endpoint);
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

    static buffer make_message(size_t size, size_t seed)
    {
        buffer result(size);
        for(size_t i = 0; i < result.size(); ++i) {
            result[i] = static_cast<std::byte>((i * 53u + seed * 31u + 17u) % 251u);
        }
        return result;
    }

    static uint16_t reserve_loopback_port()
    {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(loopback_address, 0));
        return acceptor.local_endpoint().port();
    }

    std::unique_ptr<tcp::socket> connect_loopback(uint16_t port)
    {
        auto socket = std::make_unique<tcp::socket>(m_client_io_context);
        boost::system::error_code ec;
        socket->connect(tcp::endpoint(loopback_address, port), ec);
        if(ec) {
            ADD_FAILURE() << "Loopback connect failed: " << ec.message();
            return {};
        }
        return socket;
    }

protected:
    boost::asio::io_context m_client_io_context;
    std::shared_ptr<thread_pool> m_thread_pool;
};

TEST_F(TcpPipeServerEnv, RejectsInvalidIpv4Address)
{
    EXPECT_THROW((void)server_env(m_thread_pool.get(), "256.1.2.3", 12345), boost::system::system_error);
    EXPECT_THROW((void)server_env(m_thread_pool.get(), "::1", 12345), boost::system::system_error);
    EXPECT_THROW((void)server_env(m_thread_pool.get(), "localhost", 12345), boost::system::system_error);
}

TEST_F(TcpPipeServerEnv, RejectsPortOutsideUint16Range)
{
    EXPECT_THROW((void)server_env(m_thread_pool.get(), loopback_ip, 65536), std::invalid_argument);
}

TEST_F(TcpPipeServerEnv, RejectsAddressAlreadyUsedByAnotherServer)
{
    const auto port = reserve_loopback_port();
    server_env first(m_thread_pool.get(), loopback_ip, port);
    static_cast<void>(first);

    EXPECT_THROW((void)server_env(m_thread_pool.get(), loopback_ip, port), boost::system::system_error);
}

TEST_F(TcpPipeServerEnv, CreatesConnectedEndpointOnLoopback)
{
    const auto port = reserve_loopback_port();
    server_env env(m_thread_pool.get(), loopback_ip, port);
    auto future = env.create_pipe(0);

    auto client = connect_loopback(port);
    ASSERT_TRUE(client);
    ASSERT_TRUE(wait_for_result(future,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The loopback server endpoint create operation"));

    auto result = get_endpoint_result(future);
    ASSERT_EQ(result.state, pipe_wait_res::success);
    ASSERT_TRUE(result.endpoint);
    EXPECT_TRUE(result.endpoint->is_connected());
}

TEST_F(TcpPipeServerEnv, WaitsUntilLoopbackClientConnects)
{
    const auto port = reserve_loopback_port();
    server_env env(m_thread_pool.get(), loopback_ip, port);
    auto future = env.create_pipe(0);

    EXPECT_FALSE(future.wait_for(pending_observation_timeout));

    auto client = connect_loopback(port);
    ASSERT_TRUE(client);
    ASSERT_TRUE(wait_for_result(future,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The waiting loopback accept"));

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::success);
    EXPECT_TRUE(result.endpoint);
}

TEST_F(TcpPipeServerEnv, TimedCreateSucceedsBeforeDeadline)
{
    const auto port = reserve_loopback_port();
    server_env env(m_thread_pool.get(), loopback_ip, port);
    auto future = env.create_pipe(0, long_create_timeout);

    auto client = connect_loopback(port);
    ASSERT_TRUE(client);
    ASSERT_TRUE(wait_for_result(future,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The timed loopback accept"));

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::success);
    EXPECT_TRUE(result.endpoint);
}

TEST_F(TcpPipeServerEnv, CreatedEndpointTransfersDataInBothDirections)
{
    const auto port = reserve_loopback_port();
    server_env server(m_thread_pool.get(), loopback_ip, port);
    client_env client(m_thread_pool.get(), loopback_ip, port);

    auto server_future = server.create_pipe(0);
    auto client_future = client.open_pipe(0);
    ASSERT_TRUE(wait_for_result(server_future,
                                [&] { server.cancel_all_pending_server_endpoints(); },
                                "The server side of the loopback endpoint pair"));
    ASSERT_TRUE(wait_for_result(client_future,
                                [&] { client.cancel_all_pending_client_endpoints(); },
                                "The client side of the loopback endpoint pair"));

    auto server_result = get_endpoint_result(server_future);
    auto client_result = get_endpoint_result(client_future);
    ASSERT_EQ(server_result.state, pipe_wait_res::success);
    ASSERT_EQ(client_result.state, pipe_wait_res::success);
    ASSERT_TRUE(server_result.endpoint);
    ASSERT_TRUE(client_result.endpoint);

    auto to_server = make_message(1021, 1);
    auto expected_at_server = to_server.copy();
    auto server_read = server_result.endpoint->read_async();
    auto client_write = client_result.endpoint->write_async(std::move(to_server));
    ASSERT_TRUE(wait_for_result(client_write,
                                [&] { client_result.endpoint->invalidate(); },
                                "The client-to-server write"));
    ASSERT_TRUE(wait_for_result(server_read,
                                [&] { server_result.endpoint->invalidate(); },
                                "The client-to-server read"));
    EXPECT_EQ(get_write_result(client_write), pipe_op_res::success);
    auto server_read_result = get_read_result(server_read);
    EXPECT_EQ(server_read_result.state, pipe_op_res::success);
    EXPECT_EQ(server_read_result.data, expected_at_server);

    auto to_client = make_message(1031, 2);
    auto expected_at_client = to_client.copy();
    auto client_read = client_result.endpoint->read_async();
    auto server_write = server_result.endpoint->write_async(std::move(to_client));
    ASSERT_TRUE(wait_for_result(server_write,
                                [&] { server_result.endpoint->invalidate(); },
                                "The server-to-client write"));
    ASSERT_TRUE(wait_for_result(client_read,
                                [&] { client_result.endpoint->invalidate(); },
                                "The server-to-client read"));
    EXPECT_EQ(get_write_result(server_write), pipe_op_res::success);
    auto client_read_result = get_read_result(client_read);
    EXPECT_EQ(client_read_result.state, pipe_op_res::success);
    EXPECT_EQ(client_read_result.data, expected_at_client);
}

TEST_F(TcpPipeServerEnv, TimeoutCancelsActiveAcceptWithoutClientConnection)
{
    const auto port = reserve_loopback_port();
    server_env env(m_thread_pool.get(), loopback_ip, port);

    auto future = env.create_pipe(0, pending_create_timeout);
    ASSERT_TRUE(wait_for_result(future,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The timed out active loopback accept"));

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::timeout);
    EXPECT_FALSE(result.endpoint);
}

TEST_F(TcpPipeServerEnv, ExplicitCancellationCompletesActiveAccept)
{
    const auto port = reserve_loopback_port();
    server_env env(m_thread_pool.get(), loopback_ip, port);
    auto future = env.create_pipe(0);
    ASSERT_FALSE(future.wait_for(pending_observation_timeout));

    // This exercises cancellation of a real outstanding async_accept. On a
    // Windows target where Boost.Asio cannot cancel that operation, this test
    // fails by the bounded wait below instead of silently accepting the gap.
    env.cancel_all_pending_server_endpoints();
    ASSERT_TRUE(wait_for_result(future,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The explicitly canceled active loopback accept"));

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::canceled);
    EXPECT_FALSE(result.endpoint);
}

TEST_F(TcpPipeServerEnv, QueuedCreateCanTimeOutWithoutCancelingActiveAccept)
{
    const auto port = reserve_loopback_port();
    server_env env(m_thread_pool.get(), loopback_ip, port);
    auto active = env.create_pipe(0);
    auto queued = env.create_pipe(0, pending_create_timeout);

    ASSERT_TRUE(wait_for_result(queued,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The timed out queued loopback accept"));
    auto queued_result = get_endpoint_result(queued);
    ASSERT_EQ(queued_result.state, pipe_wait_res::timeout);
    ASSERT_FALSE(queued_result.endpoint);
    EXPECT_FALSE(active.wait_for(pending_observation_timeout));

    auto client = connect_loopback(port);
    ASSERT_TRUE(client);
    ASSERT_TRUE(wait_for_result(active,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The active accept unaffected by a queued timeout"));
    auto active_result = get_endpoint_result(active);
    EXPECT_EQ(active_result.state, pipe_wait_res::success);
    EXPECT_TRUE(active_result.endpoint);
}

TEST_F(TcpPipeServerEnv, ActiveTimeoutStartsNextQueuedAccept)
{
    const auto port = reserve_loopback_port();
    server_env env(m_thread_pool.get(), loopback_ip, port);
    auto timed = env.create_pipe(0, pending_create_timeout);
    auto next = env.create_pipe(0);

    ASSERT_TRUE(wait_for_result(timed,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The active accept that must time out"));
    auto timed_result = get_endpoint_result(timed);
    ASSERT_EQ(timed_result.state, pipe_wait_res::timeout);
    ASSERT_FALSE(timed_result.endpoint);

    auto client = connect_loopback(port);
    ASSERT_TRUE(client);
    ASSERT_TRUE(wait_for_result(next,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The queued accept started after an active timeout"));
    auto next_result = get_endpoint_result(next);
    EXPECT_EQ(next_result.state, pipe_wait_res::success);
    EXPECT_TRUE(next_result.endpoint);
}

TEST_F(TcpPipeServerEnv, CancelsActiveAndQueuedCreateOperations)
{
    const auto port = reserve_loopback_port();
    server_env env(m_thread_pool.get(), loopback_ip, port);
    auto first = env.create_pipe(0, long_create_timeout);
    auto second = env.create_pipe(0);
    auto third = env.create_pipe(0, long_create_timeout);

    env.cancel_all_pending_server_endpoints();
    ASSERT_TRUE(wait_for_result(first,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The canceled active accept"));
    ASSERT_TRUE(wait_for_result(second,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The canceled queued accept without a timer"));
    ASSERT_TRUE(wait_for_result(third,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The canceled queued accept with a timer"));

    auto first_result = get_endpoint_result(first);
    auto second_result = get_endpoint_result(second);
    auto third_result = get_endpoint_result(third);
    EXPECT_EQ(first_result.state, pipe_wait_res::canceled);
    EXPECT_EQ(second_result.state, pipe_wait_res::canceled);
    EXPECT_EQ(third_result.state, pipe_wait_res::canceled);
    EXPECT_FALSE(first_result.endpoint);
    EXPECT_FALSE(second_result.endpoint);
    EXPECT_FALSE(third_result.endpoint);
}

TEST_F(TcpPipeServerEnv, CancellationIsIdempotentAndServerRemainsReusable)
{
    const auto port = reserve_loopback_port();
    server_env env(m_thread_pool.get(), loopback_ip, port);
    auto canceled = env.create_pipe(0);

    env.cancel_all_pending_server_endpoints();
    env.cancel_all_pending_server_endpoints();
    ASSERT_TRUE(wait_for_result(canceled,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The repeatedly canceled active accept"));
    auto canceled_result = get_endpoint_result(canceled);
    ASSERT_EQ(canceled_result.state, pipe_wait_res::canceled);
    ASSERT_FALSE(canceled_result.endpoint);

    env.cancel_all_pending_server_endpoints();
    auto next = env.create_pipe(0);
    auto client = connect_loopback(port);
    ASSERT_TRUE(client);
    ASSERT_TRUE(wait_for_result(next,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The accept created after cancellation"));
    auto next_result = get_endpoint_result(next);
    EXPECT_EQ(next_result.state, pipe_wait_res::success);
    EXPECT_TRUE(next_result.endpoint);
}

TEST_F(TcpPipeServerEnv, DestructorCancelsPendingActiveAccept)
{
    const auto port = reserve_loopback_port();
    endpoint_future future;
    {
        auto env = std::make_unique<server_env>(m_thread_pool.get(), loopback_ip, port);
        future = env->create_pipe(0);
        EXPECT_FALSE(future.wait_for(pending_observation_timeout));
    }

    ASSERT_TRUE(wait_for_result(future,
                                [] {},
                                "The active accept canceled by server destruction"));
    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::canceled);
    EXPECT_FALSE(result.endpoint);
}

TEST_F(TcpPipeServerEnv, CancelDoesNotAffectCompletedEndpoint)
{
    const auto port = reserve_loopback_port();
    server_env env(m_thread_pool.get(), loopback_ip, port);
    auto future = env.create_pipe(0);
    auto client = connect_loopback(port);
    ASSERT_TRUE(client);
    ASSERT_TRUE(wait_for_result(future,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The completed accept"));
    auto result = get_endpoint_result(future);
    ASSERT_EQ(result.state, pipe_wait_res::success);
    ASSERT_TRUE(result.endpoint);

    env.cancel_all_pending_server_endpoints();

    EXPECT_TRUE(result.endpoint->is_connected());
}

TEST_F(TcpPipeServerEnv, SupportsMultipleQueuedCreateOperations)
{
    const auto port = reserve_loopback_port();
    server_env env(m_thread_pool.get(), loopback_ip, port);
    auto first = env.create_pipe(0);
    auto second = env.create_pipe(0);
    auto third = env.create_pipe(0);

    std::vector<std::unique_ptr<tcp::socket>> clients;
    clients.push_back(connect_loopback(port));
    ASSERT_TRUE(clients.back());
    ASSERT_TRUE(wait_for_result(first,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The first queued loopback accept"));

    clients.push_back(connect_loopback(port));
    ASSERT_TRUE(clients.back());
    ASSERT_TRUE(wait_for_result(second,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The second queued loopback accept"));

    clients.push_back(connect_loopback(port));
    ASSERT_TRUE(clients.back());
    ASSERT_TRUE(wait_for_result(third,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The third queued loopback accept"));

    auto first_result = get_endpoint_result(first);
    auto second_result = get_endpoint_result(second);
    auto third_result = get_endpoint_result(third);
    ASSERT_EQ(first_result.state, pipe_wait_res::success);
    ASSERT_EQ(second_result.state, pipe_wait_res::success);
    ASSERT_EQ(third_result.state, pipe_wait_res::success);
    ASSERT_TRUE(first_result.endpoint);
    ASSERT_TRUE(second_result.endpoint);
    ASSERT_TRUE(third_result.endpoint);
    EXPECT_NE(first_result.endpoint, second_result.endpoint);
    EXPECT_NE(first_result.endpoint, third_result.endpoint);
    EXPECT_NE(second_result.endpoint, third_result.endpoint);
}

TEST_F(TcpPipeServerEnv, CancellationRacingWithAcceptReturnsOnlyUsableSuccessOrCanceled)
{
    for(size_t i = 0; i < cancellation_race_iteration_count; ++i) {
        const auto port = reserve_loopback_port();
        server_env env(m_thread_pool.get(), loopback_ip, port);
        auto future = env.create_pipe(0);
        auto client = connect_loopback(port);
        ASSERT_TRUE(client);

        env.cancel_all_pending_server_endpoints();
        ASSERT_TRUE(wait_for_result(future,
                                    [&] { env.cancel_all_pending_server_endpoints(); },
                                    "The accept racing with explicit cancellation"));
        auto result = get_endpoint_result(future);
        ASSERT_TRUE(result.state == pipe_wait_res::success ||
                    result.state == pipe_wait_res::canceled);

        if(result.state == pipe_wait_res::success) {
            ASSERT_TRUE(result.endpoint);
            EXPECT_TRUE(result.endpoint->is_connected());
        } else {
            EXPECT_FALSE(result.endpoint);
        }
    }
}

TEST_F(TcpPipeServerEnv, CancelingQueuedClientIdDoesNotCancelActiveAcceptOfAnotherClientId)
{
    constexpr auto active_client_id = 101u;
    constexpr auto canceled_client_id = 202u;
    const auto port = reserve_loopback_port();
    server_env env(m_thread_pool.get(), loopback_ip, port);
    auto active = env.create_pipe(active_client_id);
    auto canceled = env.create_pipe(canceled_client_id);

    env.cancel_pending_server_endpoints(canceled_client_id);

    ASSERT_TRUE(wait_for_result(canceled,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The queued accept canceled by client id"));
    auto canceled_result = get_endpoint_result(canceled);
    ASSERT_EQ(canceled_result.state, pipe_wait_res::canceled);
    ASSERT_FALSE(canceled_result.endpoint);

    auto client = connect_loopback(port);
    ASSERT_TRUE(client);
    ASSERT_TRUE(wait_for_result(active,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The active accept belonging to another client id"));
    auto active_result = get_endpoint_result(active);
    EXPECT_EQ(active_result.state, pipe_wait_res::success);
    EXPECT_TRUE(active_result.endpoint);
}

TEST_F(TcpPipeServerEnv, CancelsActiveAndQueuedAcceptsWithSpecifiedClientId)
{
    constexpr auto canceled_client_id = 101u;
    constexpr auto remaining_client_id = 202u;
    const auto port = reserve_loopback_port();
    server_env env(m_thread_pool.get(), loopback_ip, port);
    auto active_canceled = env.create_pipe(canceled_client_id);
    auto queued_canceled = env.create_pipe(canceled_client_id);
    auto remaining = env.create_pipe(remaining_client_id);

    env.cancel_pending_server_endpoints(canceled_client_id);

    ASSERT_TRUE(wait_for_result(active_canceled,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The active accept canceled by client id"));
    ASSERT_TRUE(wait_for_result(queued_canceled,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The queued accept canceled by client id"));
    auto active_result = get_endpoint_result(active_canceled);
    auto queued_result = get_endpoint_result(queued_canceled);
    ASSERT_EQ(active_result.state, pipe_wait_res::canceled);
    ASSERT_EQ(queued_result.state, pipe_wait_res::canceled);
    ASSERT_FALSE(active_result.endpoint);
    ASSERT_FALSE(queued_result.endpoint);

    auto client = connect_loopback(port);
    ASSERT_TRUE(client);
    ASSERT_TRUE(wait_for_result(remaining,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The queued accept belonging to another client id"));
    auto remaining_result = get_endpoint_result(remaining);
    EXPECT_EQ(remaining_result.state, pipe_wait_res::success);
    EXPECT_TRUE(remaining_result.endpoint);
}

TEST_F(TcpPipeServerEnv, CancelsAllPendingCreateOperationsWithDifferentClientIds)
{
    const auto port = reserve_loopback_port();
    server_env env(m_thread_pool.get(), loopback_ip, port);
    auto first = env.create_pipe(101);
    auto second = env.create_pipe(202);
    auto third = env.create_pipe(303);

    env.cancel_all_pending_server_endpoints();

    ASSERT_TRUE(wait_for_result(first,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The first accept canceled without filtering"));
    ASSERT_TRUE(wait_for_result(second,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The second accept canceled without filtering"));
    ASSERT_TRUE(wait_for_result(third,
                                [&] { env.cancel_all_pending_server_endpoints(); },
                                "The third accept canceled without filtering"));
    auto first_result = get_endpoint_result(first);
    auto second_result = get_endpoint_result(second);
    auto third_result = get_endpoint_result(third);
    EXPECT_EQ(first_result.state, pipe_wait_res::canceled);
    EXPECT_EQ(second_result.state, pipe_wait_res::canceled);
    EXPECT_EQ(third_result.state, pipe_wait_res::canceled);
    EXPECT_FALSE(first_result.endpoint);
    EXPECT_FALSE(second_result.endpoint);
    EXPECT_FALSE(third_result.endpoint);
}
