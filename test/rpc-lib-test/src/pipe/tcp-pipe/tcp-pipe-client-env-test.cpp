#include <rpc-lib/pipe/tcp-pipe/tcp-pipe-client-env.h>
#include <rpc-lib/pipe/ipipe-endpoint.h>

#include <common-lib/thread/thread-pool/thread-pool.h>

#include <boost/asio/error.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

using namespace vshalygin::cl;
using namespace vshalygin::rpc;
using namespace testing;

namespace {
    using tcp = boost::asio::ip::tcp;

    constexpr auto operation_timeout = std::chrono::seconds(5);
    constexpr auto long_open_timeout = std::chrono::seconds(30);
    constexpr auto pending_open_timeout = std::chrono::milliseconds(100);
    constexpr auto pending_observation_timeout = std::chrono::milliseconds(100);
    constexpr auto socket_poll_delay = std::chrono::milliseconds(1);
    constexpr size_t max_backlog_fill_attempts = 64;
    constexpr uint16_t unavailable_loopback_port = 0;

    const auto loopback_address = boost::asio::ip::make_address_v4("127.0.0.1");

    struct endpoint_result
    {
        pipe_wait_res state = pipe_wait_res::failed;
        std::shared_ptr<ipipe_endpoint> endpoint;
    };

    struct connect_state
    {
        boost::system::error_code error;
        std::atomic<bool> completed = false;
    };

    struct saturated_loopback_listener
    {
        explicit saturated_loopback_listener(boost::asio::io_context &io_context)
            : acceptor(io_context)
        {}

        tcp::acceptor acceptor;
        std::vector<std::unique_ptr<tcp::socket>> clients;
    };
}

class TcpPipeClientEnv
    : public Test
{
protected:
    using client_env = tcp_pipe_client_env;
    using endpoint_future = iclient_pipe_env::pipe_endpoint_future;

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
        future.get().apply([&](pipe_wait_res state, std::shared_ptr<ipipe_endpoint> endpoint) {
            result.state = state;
            result.endpoint = std::move(endpoint);
        });
        return result;
    }

    static bool is_would_block(const boost::system::error_code &ec)
    {
        return ec == boost::asio::error::would_block ||
               ec == boost::asio::error::try_again;
    }

    static bool wait_for_connect_completion(const std::shared_ptr<connect_state> &state,
                                            std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while(!state->completed.load(std::memory_order_acquire) &&
              std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(socket_poll_delay);
        }
        return state->completed.load(std::memory_order_acquire);
    }

    tcp::acceptor create_bound_loopback_acceptor()
    {
        auto &io_context = m_thread_pool->get_io_context();
        tcp::acceptor acceptor(io_context);
        acceptor.open(tcp::v4());
        acceptor.bind(tcp::endpoint(loopback_address, 0));
        return acceptor;
    }

    tcp::acceptor create_loopback_listener(
        int backlog = boost::asio::socket_base::max_listen_connections)
    {
        auto acceptor = create_bound_loopback_acceptor();
        acceptor.listen(backlog);
        return acceptor;
    }

    static bool accept_one(tcp::acceptor &acceptor, tcp::socket &peer)
    {
        boost::system::error_code ec;
        acceptor.non_blocking(true, ec);
        if(ec) {
            ADD_FAILURE() << "Failed to make the loopback acceptor non-blocking: " << ec.message();
            return false;
        }

        const auto deadline = std::chrono::steady_clock::now() + operation_timeout;
        while(std::chrono::steady_clock::now() < deadline) {
            acceptor.accept(peer, ec);
            if(!ec) {
                return true;
            }
            if(!is_would_block(ec)) {
                ADD_FAILURE() << "Loopback accept failed: " << ec.message();
                return false;
            }
            std::this_thread::sleep_for(socket_poll_delay);
        }

        ADD_FAILURE() << "Loopback accept did not complete in time";
        return false;
    }

    std::unique_ptr<saturated_loopback_listener> create_saturated_loopback_listener()
    {
        auto &io_context = m_thread_pool->get_io_context();
        auto result = std::make_unique<saturated_loopback_listener>(io_context);

        boost::system::error_code ec;
        result->acceptor.open(tcp::v4(), ec);
        if(ec) {
            ADD_FAILURE() << "Failed to open the loopback acceptor: " << ec.message();
            return {};
        }
        result->acceptor.bind(tcp::endpoint(loopback_address, 0), ec);
        if(ec) {
            ADD_FAILURE() << "Failed to bind the loopback acceptor: " << ec.message();
            return {};
        }
        result->acceptor.listen(1, ec);
        if(ec) {
            ADD_FAILURE() << "Failed to listen on the loopback acceptor: " << ec.message();
            return {};
        }

        const auto endpoint = result->acceptor.local_endpoint();
        result->clients.reserve(max_backlog_fill_attempts);

        for(size_t i = 0; i < max_backlog_fill_attempts; ++i) {
            auto client = std::make_unique<tcp::socket>(io_context);
            auto state = std::make_shared<connect_state>();

            client->async_connect(endpoint, [state](const boost::system::error_code &connect_ec) {
                state->error = connect_ec;
                state->completed.store(true, std::memory_order_release);
            });
            result->clients.push_back(std::move(client));

            if(!wait_for_connect_completion(state, pending_observation_timeout)) {
                return result;
            }
            if(state->error) {
                ADD_FAILURE() << "A backlog filler connect failed: " << state->error.message();
                return {};
            }
        }

        ADD_FAILURE() << "Could not saturate the loopback listen queue";
        return {};
    }

protected:
    std::shared_ptr<thread_pool> m_thread_pool;
};

TEST_F(TcpPipeClientEnv, RejectsInvalidIpv4Address)
{
    EXPECT_THROW((void)client_env(m_thread_pool.get(), "256.1.2.3", 12345), std::invalid_argument);
    EXPECT_THROW((void)client_env(m_thread_pool.get(), "::1", 12345), std::invalid_argument);
    EXPECT_THROW((void)client_env(m_thread_pool.get(), "localhost", 12345), std::invalid_argument);
}

TEST_F(TcpPipeClientEnv, RejectsPortOutsideUint16Range)
{
    EXPECT_THROW((void)client_env(m_thread_pool.get(), "127.0.0.1", 65536), std::invalid_argument);
}

TEST_F(TcpPipeClientEnv, OpensConnectedEndpointOnLoopback)
{
    auto acceptor = create_loopback_listener();
    client_env env(m_thread_pool.get(), "127.0.0.1", acceptor.local_endpoint().port());

    auto future = env.open_pipe(0);
    tcp::socket peer(m_thread_pool->get_io_context());
    ASSERT_TRUE(accept_one(acceptor, peer));
    ASSERT_TRUE(wait_for_result(future,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The loopback client endpoint open operation"));

    auto result = get_endpoint_result(future);
    ASSERT_EQ(result.state, pipe_wait_res::success);
    ASSERT_TRUE(result.endpoint);
    EXPECT_TRUE(result.endpoint->is_connected());
}

TEST_F(TcpPipeClientEnv, TimedOpenSucceedsBeforeDeadline)
{
    auto acceptor = create_loopback_listener();
    client_env env(m_thread_pool.get(), "127.0.0.1", acceptor.local_endpoint().port());

    auto future = env.open_pipe(0, long_open_timeout);
    tcp::socket peer(m_thread_pool->get_io_context());
    ASSERT_TRUE(accept_one(acceptor, peer));
    ASSERT_TRUE(wait_for_result(future,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The timed loopback client endpoint open operation"));

    auto result = get_endpoint_result(future);
    ASSERT_EQ(result.state, pipe_wait_res::success);
    ASSERT_TRUE(result.endpoint);
    EXPECT_TRUE(result.endpoint->is_connected());
}

TEST_F(TcpPipeClientEnv, ReportsFailureWhenLoopbackPortHasNoListener)
{
    client_env env(m_thread_pool.get(), "127.0.0.1", unavailable_loopback_port);

    auto future = env.open_pipe(0);
    ASSERT_TRUE(wait_for_result(future,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The refused loopback connect"));

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::failed);
    EXPECT_FALSE(result.endpoint);
}

TEST_F(TcpPipeClientEnv, TimedOpenReportsFailureBeforeDeadline)
{
    client_env env(m_thread_pool.get(), "127.0.0.1", unavailable_loopback_port);

    auto future = env.open_pipe(0, long_open_timeout);
    ASSERT_TRUE(wait_for_result(future,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The timed refused loopback connect"));

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::failed);
    EXPECT_FALSE(result.endpoint);
}

TEST_F(TcpPipeClientEnv, TimesOutPendingLoopbackConnect)
{
    auto listener = create_saturated_loopback_listener();
    ASSERT_TRUE(listener);
    client_env env(m_thread_pool.get(), "127.0.0.1", listener->acceptor.local_endpoint().port());

    auto future = env.open_pipe(0, pending_open_timeout);
    ASSERT_TRUE(wait_for_result(future,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The timed out loopback connect"));

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::timeout);
    EXPECT_FALSE(result.endpoint);
}

TEST_F(TcpPipeClientEnv, CancelPendingOpenIsIdempotent)
{
    auto listener = create_saturated_loopback_listener();
    ASSERT_TRUE(listener);
    client_env env(m_thread_pool.get(), "127.0.0.1", listener->acceptor.local_endpoint().port());

    auto future = env.open_pipe(0);
    EXPECT_FALSE(future.wait_for(pending_observation_timeout));

    env.cancel_all_pending_client_endpoints();
    env.cancel_all_pending_client_endpoints();
    ASSERT_TRUE(wait_for_result(future,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The canceled loopback connect"));

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::canceled);
    EXPECT_FALSE(result.endpoint);
}

TEST_F(TcpPipeClientEnv, CancelsAllPendingOpenOperations)
{
    auto listener = create_saturated_loopback_listener();
    ASSERT_TRUE(listener);
    client_env env(m_thread_pool.get(), "127.0.0.1", listener->acceptor.local_endpoint().port());

    auto first = env.open_pipe(0);
    auto second = env.open_pipe(0);
    auto third = env.open_pipe(0);
    env.cancel_all_pending_client_endpoints();

    ASSERT_TRUE(wait_for_result(first, [&] { env.cancel_all_pending_client_endpoints(); }, "The first canceled connect"));
    ASSERT_TRUE(wait_for_result(second, [&] { env.cancel_all_pending_client_endpoints(); }, "The second canceled connect"));
    ASSERT_TRUE(wait_for_result(third, [&] { env.cancel_all_pending_client_endpoints(); }, "The third canceled connect"));

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

TEST_F(TcpPipeClientEnv, DestructorCancelsPendingOpen)
{
    auto listener = create_saturated_loopback_listener();
    ASSERT_TRUE(listener);

    endpoint_future future;
    {
        auto env = std::make_unique<client_env>(m_thread_pool.get(),
                                                "127.0.0.1",
                                                listener->acceptor.local_endpoint().port());
        future = env->open_pipe(0);
        EXPECT_FALSE(future.wait_for(pending_observation_timeout));
    }

    ASSERT_TRUE(wait_for_result(future,
                                [&] { listener.reset(); },
                                "The connect canceled by tcp_pipe_client_env destruction"));

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::canceled);
    EXPECT_FALSE(result.endpoint);
}

TEST_F(TcpPipeClientEnv, TimeoutOfOneOpenDoesNotCancelAnother)
{
    auto listener = create_saturated_loopback_listener();
    ASSERT_TRUE(listener);
    client_env env(m_thread_pool.get(), "127.0.0.1", listener->acceptor.local_endpoint().port());

    auto pending = env.open_pipe(0);
    auto timed = env.open_pipe(0, pending_open_timeout);

    ASSERT_TRUE(wait_for_result(timed,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The independently timed out loopback connect"));
    auto timed_result = get_endpoint_result(timed);
    ASSERT_EQ(timed_result.state, pipe_wait_res::timeout);
    ASSERT_FALSE(timed_result.endpoint);

    EXPECT_FALSE(pending.wait_for(pending_observation_timeout));
    env.cancel_all_pending_client_endpoints();
    ASSERT_TRUE(wait_for_result(pending,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The connect unaffected by another timeout"));

    auto pending_result = get_endpoint_result(pending);
    EXPECT_EQ(pending_result.state, pipe_wait_res::canceled);
    EXPECT_FALSE(pending_result.endpoint);
}

TEST_F(TcpPipeClientEnv, CancelDoesNotAffectCompletedEndpoint)
{
    auto acceptor = create_loopback_listener();
    client_env env(m_thread_pool.get(), "127.0.0.1", acceptor.local_endpoint().port());

    auto future = env.open_pipe(0);
    tcp::socket peer(m_thread_pool->get_io_context());
    ASSERT_TRUE(accept_one(acceptor, peer));
    ASSERT_TRUE(wait_for_result(future,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The completed loopback connect"));
    auto result = get_endpoint_result(future);
    ASSERT_EQ(result.state, pipe_wait_res::success);
    ASSERT_TRUE(result.endpoint);

    env.cancel_all_pending_client_endpoints();

    EXPECT_TRUE(result.endpoint->is_connected());
}

TEST_F(TcpPipeClientEnv, SupportsMultipleConcurrentOpenOperations)
{
    auto acceptor = create_loopback_listener();
    client_env env(m_thread_pool.get(), "127.0.0.1", acceptor.local_endpoint().port());

    auto first = env.open_pipe(0);
    auto second = env.open_pipe(0);
    auto third = env.open_pipe(0);

    tcp::socket first_peer(m_thread_pool->get_io_context());
    tcp::socket second_peer(m_thread_pool->get_io_context());
    tcp::socket third_peer(m_thread_pool->get_io_context());
    ASSERT_TRUE(accept_one(acceptor, first_peer));
    ASSERT_TRUE(accept_one(acceptor, second_peer));
    ASSERT_TRUE(accept_one(acceptor, third_peer));

    ASSERT_TRUE(wait_for_result(first, [&] { env.cancel_all_pending_client_endpoints(); }, "The first loopback connect"));
    ASSERT_TRUE(wait_for_result(second, [&] { env.cancel_all_pending_client_endpoints(); }, "The second loopback connect"));
    ASSERT_TRUE(wait_for_result(third, [&] { env.cancel_all_pending_client_endpoints(); }, "The third loopback connect"));

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

TEST_F(TcpPipeClientEnv, CancelRacingWithSuccessfulConnectNeverReturnsClosedSuccess)
{
    constexpr size_t iteration_count = 32;

    for(size_t i = 0; i < iteration_count; ++i) {
        auto acceptor = create_loopback_listener();
        client_env env(m_thread_pool.get(), "127.0.0.1", acceptor.local_endpoint().port());

        auto future = env.open_pipe(0);
        tcp::socket peer(m_thread_pool->get_io_context());
        ASSERT_TRUE(accept_one(acceptor, peer));

        env.cancel_all_pending_client_endpoints();
        ASSERT_TRUE(wait_for_result(future,
                                    [&] { env.cancel_all_pending_client_endpoints(); },
                                    "The connect racing with cancellation"));

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

TEST_F(TcpPipeClientEnv, CancelsOnlyPendingOpenOperationsWithSpecifiedClientId)
{
    constexpr auto canceled_client_id = 101u;
    constexpr auto remaining_client_id = 202u;
    auto listener = create_saturated_loopback_listener();
    ASSERT_TRUE(listener);
    client_env env(m_thread_pool.get(), "127.0.0.1", listener->acceptor.local_endpoint().port());

    auto first_canceled = env.open_pipe(canceled_client_id);
    auto second_canceled = env.open_pipe(canceled_client_id);
    auto remaining = env.open_pipe(remaining_client_id);
    ASSERT_FALSE(remaining.wait_for(pending_observation_timeout));

    env.cancel_pending_client_endpoints(canceled_client_id);

    ASSERT_TRUE(wait_for_result(first_canceled,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The first connect canceled by client id"));
    ASSERT_TRUE(wait_for_result(second_canceled,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The second connect canceled by client id"));
    auto first_result = get_endpoint_result(first_canceled);
    auto second_result = get_endpoint_result(second_canceled);
    ASSERT_EQ(first_result.state, pipe_wait_res::canceled);
    ASSERT_EQ(second_result.state, pipe_wait_res::canceled);
    ASSERT_FALSE(first_result.endpoint);
    ASSERT_FALSE(second_result.endpoint);

    std::vector<std::unique_ptr<tcp::socket>> accepted_peers;
    boost::system::error_code ec;
    listener->acceptor.non_blocking(true, ec);
    ASSERT_FALSE(ec) << "Failed to make the loopback acceptor non-blocking: " << ec.message();

    const auto deadline = std::chrono::steady_clock::now() + operation_timeout;
    while(!remaining.wait_for(std::chrono::milliseconds(0)) &&
          std::chrono::steady_clock::now() < deadline) {
        auto peer = std::make_unique<tcp::socket>(m_thread_pool->get_io_context());
        listener->acceptor.accept(*peer, ec);
        if(!ec) {
            accepted_peers.push_back(std::move(peer));
        } else if(is_would_block(ec)) {
            std::this_thread::sleep_for(socket_poll_delay);
        } else {
            FAIL() << "Loopback accept failed: " << ec.message();
        }
    }

    ASSERT_TRUE(wait_for_result(remaining,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The connect belonging to another client id"));
    auto remaining_result = get_endpoint_result(remaining);
    EXPECT_EQ(remaining_result.state, pipe_wait_res::success);
    ASSERT_TRUE(remaining_result.endpoint);
    EXPECT_TRUE(remaining_result.endpoint->is_connected());
}

TEST_F(TcpPipeClientEnv, CancelsAllPendingOpenOperationsWithDifferentClientIds)
{
    auto listener = create_saturated_loopback_listener();
    ASSERT_TRUE(listener);
    client_env env(m_thread_pool.get(), "127.0.0.1", listener->acceptor.local_endpoint().port());

    auto first = env.open_pipe(101);
    auto second = env.open_pipe(202);
    auto third = env.open_pipe(303);

    env.cancel_all_pending_client_endpoints();

    ASSERT_TRUE(wait_for_result(first,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The first connect canceled without filtering"));
    ASSERT_TRUE(wait_for_result(second,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The second connect canceled without filtering"));
    ASSERT_TRUE(wait_for_result(third,
                                [&] { env.cancel_all_pending_client_endpoints(); },
                                "The third connect canceled without filtering"));
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
