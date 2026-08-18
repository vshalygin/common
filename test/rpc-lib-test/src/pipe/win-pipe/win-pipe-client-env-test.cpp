#ifdef _WIN32
#include <rpc-lib/pipe/win-pipe/win-pipe-client-env.h>
#include <rpc-lib/pipe/ipipe-endpoint.h>
#include <rpc-lib/consts.h>

#include <win-lib/types/handle.h>

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
using namespace vshalygin::win;
using namespace testing;

namespace {
    constexpr auto operation_timeout = std::chrono::seconds(5);
    constexpr auto retry_observation_timeout = std::chrono::milliseconds(200);
    constexpr auto immediate_timeout = std::chrono::milliseconds(0);

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

class WinPipeClientEnv
    : public Test
{
protected:
    using client_env = win_pipe_client_pipe_env;
    using endpoint_future = iclient_pipe_env::pipe_endpoint_future;
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

    static std::wstring make_pipe_name()
    {
        static std::atomic<std::uint64_t> next_id = 0;

        return L"rpc-lib-test-client-env-"
            + std::to_wstring(::GetCurrentProcessId())
            + L"-"
            + std::to_wstring(next_id.fetch_add(1, std::memory_order_relaxed));
    }

    static std::wstring make_full_pipe_name(const std::wstring &pipe_name)
    {
        return L"\\\\.\\pipe\\" + pipe_name;
    }

    static pipe_handle create_server(const std::wstring &pipe_name,
                                     DWORD max_instances = PIPE_UNLIMITED_INSTANCES)
    {
        auto buffer_size = static_cast<DWORD>(MaxTransferMessageSize * 2u);
        return pipe_handle(::CreateNamedPipeW(make_full_pipe_name(pipe_name).c_str(),
                                              PIPE_ACCESS_DUPLEX,
                                              PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                              max_instances,
                                              buffer_size,
                                              buffer_size,
                                              0,
                                              nullptr));
    }

    static buffer make_message(size_t size, size_t seed = 0)
    {
        buffer result(size);
        for(size_t i = 0; i < result.size(); ++i) {
            result[i] = static_cast<std::byte>((i * 53u + seed * 31u + 7u) % 251u);
        }
        return result;
    }

    template<typename Future, typename Cancel>
    static void wait_for_result(Future &future, Cancel &&cancel, const char *operation_name)
    {
        if(!future.wait_for(operation_timeout)) {
            ADD_FAILURE() << operation_name << " did not complete in time";
            cancel();
            future.wait();
        }
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

    static endpoint_read_result get_read_result(read_future &future)
    {
        endpoint_read_result result;
        future.get().apply([&](pipe_op_res state, buffer &&data) {
            result.state = state;
            result.data = std::move(data);
        });
        return result;
    }

    static pipe_op_res get_write_result(write_future &future)
    {
        auto result = pipe_op_res::failed;
        future.get().apply([&](pipe_op_res state) { result = state; });
        return result;
    }

    static bool read_from_client(const pipe_handle &server, buffer &message)
    {
        DWORD bytes_read = 0;
        auto success = ::ReadFile(server.get(),
                                  message.data(),
                                  static_cast<DWORD>(message.size()),
                                  &bytes_read,
                                  nullptr) != FALSE;
        EXPECT_TRUE(success) << "Server ReadFile failed, error=" << ::GetLastError();
        EXPECT_EQ(bytes_read, message.size());
        return success && bytes_read == message.size();
    }

    static bool write_to_client(const pipe_handle &server, const buffer &message)
    {
        DWORD bytes_written = 0;
        auto success = ::WriteFile(server.get(),
                                   message.data(),
                                   static_cast<DWORD>(message.size()),
                                   &bytes_written,
                                   nullptr) != FALSE;
        EXPECT_TRUE(success) << "Server WriteFile failed, error=" << ::GetLastError();
        EXPECT_EQ(bytes_written, message.size());
        return success && bytes_written == message.size();
    }

protected:
    std::shared_ptr<thread_pool> m_thread_pool;
};

TEST_F(WinPipeClientEnv, OpensConnectedEndpointWhenServerExists)
{
    auto pipe_name = make_pipe_name();
    auto server = create_server(pipe_name);
    ASSERT_FALSE(server.empty()) << "CreateNamedPipeW failed, error=" << ::GetLastError();
    client_env env(m_thread_pool, pipe_name);

    auto future = env.open_pipe();
    wait_for_result(future,
                    [&] { env.cancel_pending_client_endpoints(); },
                    "The client endpoint open operation");

    auto result = get_endpoint_result(future);
    ASSERT_EQ(result.state, pipe_wait_res::success);
    ASSERT_TRUE(result.endpoint);
    EXPECT_TRUE(result.endpoint->is_connected());
}

TEST_F(WinPipeClientEnv, OpenedEndpointTransfersDataInBothDirections)
{
    auto pipe_name = make_pipe_name();
    auto server = create_server(pipe_name);
    ASSERT_FALSE(server.empty()) << "CreateNamedPipeW failed, error=" << ::GetLastError();
    client_env env(m_thread_pool, pipe_name);
    auto open_future = env.open_pipe();
    wait_for_result(open_future,
                    [&] { env.cancel_pending_client_endpoints(); },
                    "The client endpoint open operation");
    auto open_result = get_endpoint_result(open_future);
    ASSERT_EQ(open_result.state, pipe_wait_res::success);
    ASSERT_TRUE(open_result.endpoint);

    auto to_server = make_message(8192u * 3u + 17u, 1);
    auto expected_at_server = to_server.copy();
    auto write_future = open_result.endpoint->write_async(std::move(to_server));
    wait_for_result(write_future,
                    [&] { open_result.endpoint->invalidate(); },
                    "The client-to-server write");
    ASSERT_EQ(get_write_result(write_future), pipe_op_res::success);
    auto received_at_server = buffer(expected_at_server.size());
    ASSERT_TRUE(read_from_client(server, received_at_server));
    EXPECT_EQ(received_at_server, expected_at_server);

    auto to_client = make_message(8192u * 3u + 29u, 2);
    auto expected_at_client = to_client.copy();
    ASSERT_TRUE(write_to_client(server, to_client));
    auto read_future = open_result.endpoint->read_async();
    wait_for_result(read_future,
                    [&] { open_result.endpoint->invalidate(); },
                    "The server-to-client read");
    auto read_result = get_read_result(read_future);
    EXPECT_EQ(read_result.state, pipe_op_res::success);
    EXPECT_EQ(read_result.data, expected_at_client);
}

TEST_F(WinPipeClientEnv, RetriesUntilServerAppears)
{
    auto pipe_name = make_pipe_name();
    client_env env(m_thread_pool, pipe_name);
    auto future = env.open_pipe();

    EXPECT_FALSE(future.wait_for(retry_observation_timeout));

    auto server = create_server(pipe_name);
    ASSERT_FALSE(server.empty()) << "CreateNamedPipeW failed, error=" << ::GetLastError();
    wait_for_result(future,
                    [&] { env.cancel_pending_client_endpoints(); },
                    "The retried client endpoint open operation");

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::success);
    EXPECT_TRUE(result.endpoint);
}

TEST_F(WinPipeClientEnv, TimedOpenSucceedsWhenServerExists)
{
    auto pipe_name = make_pipe_name();
    auto server = create_server(pipe_name);
    ASSERT_FALSE(server.empty()) << "CreateNamedPipeW failed, error=" << ::GetLastError();
    client_env env(m_thread_pool, pipe_name);

    auto future = env.open_pipe(operation_timeout);
    wait_for_result(future,
                    [&] { env.cancel_pending_client_endpoints(); },
                    "The timed client endpoint open operation");

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::success);
    EXPECT_TRUE(result.endpoint);
}

TEST_F(WinPipeClientEnv, TimesOutWhenServerDoesNotExist)
{
    client_env env(m_thread_pool, make_pipe_name());

    auto future = env.open_pipe(immediate_timeout);
    wait_for_result(future,
                    [&] { env.cancel_pending_client_endpoints(); },
                    "The timed out client endpoint open operation");

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::timeout);
    EXPECT_FALSE(result.endpoint);
}

TEST_F(WinPipeClientEnv, CancelsAllPendingOpenOperations)
{
    client_env env(m_thread_pool, make_pipe_name());
    auto first = env.open_pipe();
    auto second = env.open_pipe();
    auto third = env.open_pipe();

    env.cancel_pending_client_endpoints();
    wait_for_result(first, [&] { env.cancel_pending_client_endpoints(); }, "The first canceled open");
    wait_for_result(second, [&] { env.cancel_pending_client_endpoints(); }, "The second canceled open");
    wait_for_result(third, [&] { env.cancel_pending_client_endpoints(); }, "The third canceled open");

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

TEST_F(WinPipeClientEnv, CancelDoesNotAffectCompletedEndpoint)
{
    auto pipe_name = make_pipe_name();
    auto server = create_server(pipe_name);
    ASSERT_FALSE(server.empty()) << "CreateNamedPipeW failed, error=" << ::GetLastError();
    client_env env(m_thread_pool, pipe_name);
    auto future = env.open_pipe();
    wait_for_result(future,
                    [&] { env.cancel_pending_client_endpoints(); },
                    "The completed client endpoint open operation");
    auto result = get_endpoint_result(future);
    ASSERT_EQ(result.state, pipe_wait_res::success);
    ASSERT_TRUE(result.endpoint);

    env.cancel_pending_client_endpoints();

    EXPECT_TRUE(result.endpoint->is_connected());
    auto message = make_message(127);
    auto expected = message.copy();
    auto write = result.endpoint->write_async(std::move(message));
    wait_for_result(write,
                    [&] { result.endpoint->invalidate(); },
                    "The write after canceling completed opens");
    ASSERT_EQ(get_write_result(write), pipe_op_res::success);
    auto received = buffer(expected.size());
    ASSERT_TRUE(read_from_client(server, received));
    EXPECT_EQ(received, expected);
}

TEST_F(WinPipeClientEnv, DestructorCancelsPendingOpen)
{
    auto env = std::make_unique<client_env>(m_thread_pool, make_pipe_name());
    auto future = env->open_pipe();

    env.reset();
    wait_for_result(future, [] {}, "The open canceled by client env destruction");

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::canceled);
    EXPECT_FALSE(result.endpoint);
}

TEST_F(WinPipeClientEnv, TimeoutOfOneOpenDoesNotCancelAnother)
{
    auto pipe_name = make_pipe_name();
    client_env env(m_thread_pool, pipe_name);
    auto pending = env.open_pipe();
    auto timed = env.open_pipe(immediate_timeout);

    wait_for_result(timed,
                    [&] { env.cancel_pending_client_endpoints(); },
                    "The independently timed out open");
    auto timed_result = get_endpoint_result(timed);
    ASSERT_EQ(timed_result.state, pipe_wait_res::timeout);
    ASSERT_FALSE(timed_result.endpoint);

    auto server = create_server(pipe_name);
    ASSERT_FALSE(server.empty()) << "CreateNamedPipeW failed, error=" << ::GetLastError();
    wait_for_result(pending,
                    [&] { env.cancel_pending_client_endpoints(); },
                    "The open unaffected by another timeout");
    auto pending_result = get_endpoint_result(pending);
    EXPECT_EQ(pending_result.state, pipe_wait_res::success);
    EXPECT_TRUE(pending_result.endpoint);
}

TEST_F(WinPipeClientEnv, SupportsMultipleConcurrentOpenOperations)
{
    auto pipe_name = make_pipe_name();
    auto first_server = create_server(pipe_name);
    auto second_server = create_server(pipe_name);
    ASSERT_FALSE(first_server.empty()) << "First CreateNamedPipeW failed, error=" << ::GetLastError();
    ASSERT_FALSE(second_server.empty()) << "Second CreateNamedPipeW failed, error=" << ::GetLastError();
    client_env env(m_thread_pool, pipe_name);

    auto first = env.open_pipe();
    auto second = env.open_pipe();
    wait_for_result(first, [&] { env.cancel_pending_client_endpoints(); }, "The first concurrent open");
    wait_for_result(second, [&] { env.cancel_pending_client_endpoints(); }, "The second concurrent open");

    auto first_result = get_endpoint_result(first);
    auto second_result = get_endpoint_result(second);
    EXPECT_EQ(first_result.state, pipe_wait_res::success);
    EXPECT_EQ(second_result.state, pipe_wait_res::success);
    EXPECT_TRUE(first_result.endpoint);
    EXPECT_TRUE(second_result.endpoint);
    EXPECT_NE(first_result.endpoint, second_result.endpoint);
}

TEST_F(WinPipeClientEnv, InvalidPipeNameFailsOpen)
{
    client_env env(m_thread_pool, std::wstring(512, L'x'));

    auto future = env.open_pipe();
    wait_for_result(future,
                    [&] { env.cancel_pending_client_endpoints(); },
                    "The invalid-name client endpoint open operation");

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::failed);
    EXPECT_FALSE(result.endpoint);
}
#endif
