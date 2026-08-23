#ifdef _WIN32
#include <rpc-lib/pipe/win-pipe/win-pipe-server-env.h>
#include <rpc-lib/pipe/ipipe-endpoint.h>

#include <win-lib/types/handle.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <utility>

using namespace vshalygin::cl;
using namespace vshalygin::rpc;
using namespace vshalygin::win;
using namespace testing;

namespace {
    constexpr auto operation_timeout = std::chrono::seconds(5);
    constexpr auto retry_delay = std::chrono::milliseconds(10);
    constexpr auto pending_observation_timeout = std::chrono::milliseconds(200);
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

class WinPipeServerEnv
    : public Test
{
protected:
    using server_env = win_pipe_server_env;
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

    static std::wstring make_pipe_name()
    {
        static std::atomic<std::uint64_t> next_id = 0;

        return L"rpc-lib-test-server-env-"
            + std::to_wstring(::GetCurrentProcessId())
            + L"-"
            + std::to_wstring(next_id.fetch_add(1, std::memory_order_relaxed));
    }

    static std::wstring make_full_pipe_name(const std::wstring &pipe_name)
    {
        return L"\\\\.\\pipe\\" + pipe_name;
    }

    static pipe_handle open_client(const std::wstring &pipe_name)
    {
        auto full_name = make_full_pipe_name(pipe_name);
        auto deadline = std::chrono::steady_clock::now() + operation_timeout;

        while(std::chrono::steady_clock::now() < deadline) {
            pipe_handle client(::CreateFileW(full_name.c_str(),
                                             GENERIC_READ | GENERIC_WRITE,
                                             0,
                                             nullptr,
                                             OPEN_EXISTING,
                                             0,
                                             nullptr));
            if(!client.empty()) {
                DWORD read_mode = PIPE_READMODE_MESSAGE;
                if(!::SetNamedPipeHandleState(client.get(), &read_mode, nullptr, nullptr)) {
                    return {};
                }
                return client;
            }

            auto error = ::GetLastError();
            if(error != ERROR_FILE_NOT_FOUND && error != ERROR_PIPE_BUSY) {
                return {};
            }

            if(error == ERROR_PIPE_BUSY) {
                ::WaitNamedPipeW(full_name.c_str(), static_cast<DWORD>(retry_delay.count()));
            } else {
                std::this_thread::sleep_for(retry_delay);
            }
        }

        return {};
    }

    static buffer make_message(size_t size, size_t seed = 0)
    {
        buffer result(size);
        for(size_t i = 0; i < result.size(); ++i) {
            result[i] = static_cast<std::byte>((i * 59u + seed * 37u + 13u) % 251u);
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

    static bool read_from_server(const pipe_handle &client, buffer &message)
    {
        DWORD bytes_read = 0;
        auto success = ::ReadFile(client.get(),
                                  message.data(),
                                  static_cast<DWORD>(message.size()),
                                  &bytes_read,
                                  nullptr) != FALSE;
        EXPECT_TRUE(success) << "Client ReadFile failed, error=" << ::GetLastError();
        EXPECT_EQ(bytes_read, message.size());
        return success && bytes_read == message.size();
    }

    static bool write_to_server(const pipe_handle &client, const buffer &message)
    {
        DWORD bytes_written = 0;
        auto success = ::WriteFile(client.get(),
                                   message.data(),
                                   static_cast<DWORD>(message.size()),
                                   &bytes_written,
                                   nullptr) != FALSE;
        EXPECT_TRUE(success) << "Client WriteFile failed, error=" << ::GetLastError();
        EXPECT_EQ(bytes_written, message.size());
        return success && bytes_written == message.size();
    }

protected:
    std::shared_ptr<thread_pool> m_thread_pool;
};

TEST_F(WinPipeServerEnv, CreatesConnectedEndpointWhenClientConnects)
{
    auto pipe_name = make_pipe_name();
    server_env env(m_thread_pool.get(), pipe_name);
    auto future = env.create_pipe(0);

    auto client = open_client(pipe_name);
    if(client.empty()) {
        ADD_FAILURE() << "CreateFileW did not connect to the server pipe";
        env.cancel_all_pending_server_endpoints();
    }
    wait_for_result(future,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The server endpoint create operation");

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::success);
    EXPECT_TRUE(result.endpoint);
    if(result.endpoint) {
        EXPECT_TRUE(result.endpoint->is_connected());
    }
}

TEST_F(WinPipeServerEnv, CreatedEndpointTransfersDataInBothDirections)
{
    auto pipe_name = make_pipe_name();
    server_env env(m_thread_pool.get(), pipe_name);
    auto create_future = env.create_pipe(0);
    auto client = open_client(pipe_name);
    if(client.empty()) {
        ADD_FAILURE() << "CreateFileW did not connect to the server pipe";
        env.cancel_all_pending_server_endpoints();
    }
    wait_for_result(create_future,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The server endpoint create operation");
    auto create_result = get_endpoint_result(create_future);
    ASSERT_EQ(create_result.state, pipe_wait_res::success);
    ASSERT_TRUE(create_result.endpoint);
    ASSERT_FALSE(client.empty());

    auto to_client = make_message(1021, 1);
    auto expected_at_client = to_client.copy();
    auto write_future = create_result.endpoint->write_async(std::move(to_client));
    wait_for_result(write_future,
                    [&] { create_result.endpoint->invalidate(); },
                    "The server-to-client write");
    ASSERT_EQ(get_write_result(write_future), pipe_op_res::success);
    auto received_at_client = buffer(expected_at_client.size());
    ASSERT_TRUE(read_from_server(client, received_at_client));
    EXPECT_EQ(received_at_client, expected_at_client);

    auto to_server = make_message(1031, 2);
    auto expected_at_server = to_server.copy();
    ASSERT_TRUE(write_to_server(client, to_server));
    auto read_future = create_result.endpoint->read_async();
    wait_for_result(read_future,
                    [&] { create_result.endpoint->invalidate(); },
                    "The client-to-server read");
    auto read_result = get_read_result(read_future);
    EXPECT_EQ(read_result.state, pipe_op_res::success);
    EXPECT_EQ(read_result.data, expected_at_server);
}

TEST_F(WinPipeServerEnv, WaitsForClientConnection)
{
    auto pipe_name = make_pipe_name();
    server_env env(m_thread_pool.get(), pipe_name);
    auto future = env.create_pipe(0);

    EXPECT_FALSE(future.wait_for(pending_observation_timeout));

    auto client = open_client(pipe_name);
    if(client.empty()) {
        ADD_FAILURE() << "CreateFileW did not connect to the waiting server pipe";
        env.cancel_all_pending_server_endpoints();
    }
    wait_for_result(future,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The waiting server endpoint create operation");

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::success);
    EXPECT_TRUE(result.endpoint);
}

TEST_F(WinPipeServerEnv, TimedCreateSucceedsWhenClientConnects)
{
    auto pipe_name = make_pipe_name();
    server_env env(m_thread_pool.get(), pipe_name);
    auto future = env.create_pipe(0, operation_timeout);

    auto client = open_client(pipe_name);
    if(client.empty()) {
        ADD_FAILURE() << "CreateFileW did not connect to the timed server pipe";
        env.cancel_all_pending_server_endpoints();
    }
    wait_for_result(future,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The timed server endpoint create operation");

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::success);
    EXPECT_TRUE(result.endpoint);
}

TEST_F(WinPipeServerEnv, TimesOutWithoutClientConnection)
{
    server_env env(m_thread_pool.get(), make_pipe_name());

    auto future = env.create_pipe(0, immediate_timeout);
    wait_for_result(future,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The timed out server endpoint create operation");

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::timeout);
    EXPECT_FALSE(result.endpoint);
}

TEST_F(WinPipeServerEnv, CancelsAllPendingCreateOperations)
{
    server_env env(m_thread_pool.get(), make_pipe_name());
    auto first = env.create_pipe(0);
    auto second = env.create_pipe(0);
    auto third = env.create_pipe(0);

    env.cancel_all_pending_server_endpoints();
    wait_for_result(first, [&] { env.cancel_all_pending_server_endpoints(); }, "The first canceled create");
    wait_for_result(second, [&] { env.cancel_all_pending_server_endpoints(); }, "The second canceled create");
    wait_for_result(third, [&] { env.cancel_all_pending_server_endpoints(); }, "The third canceled create");

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

TEST_F(WinPipeServerEnv, CancelDoesNotAffectCompletedEndpoint)
{
    auto pipe_name = make_pipe_name();
    server_env env(m_thread_pool.get(), pipe_name);
    auto future = env.create_pipe(0);
    auto client = open_client(pipe_name);
    if(client.empty()) {
        ADD_FAILURE() << "CreateFileW did not connect to the server pipe";
        env.cancel_all_pending_server_endpoints();
    }
    wait_for_result(future,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The completed server endpoint create operation");
    auto result = get_endpoint_result(future);
    ASSERT_EQ(result.state, pipe_wait_res::success);
    ASSERT_TRUE(result.endpoint);
    ASSERT_FALSE(client.empty());

    env.cancel_all_pending_server_endpoints();

    EXPECT_TRUE(result.endpoint->is_connected());
    auto message = make_message(131);
    auto expected = message.copy();
    auto write = result.endpoint->write_async(std::move(message));
    wait_for_result(write,
                    [&] { result.endpoint->invalidate(); },
                    "The write after canceling completed creates");
    ASSERT_EQ(get_write_result(write), pipe_op_res::success);
    auto received = buffer(expected.size());
    ASSERT_TRUE(read_from_server(client, received));
    EXPECT_EQ(received, expected);
}

TEST_F(WinPipeServerEnv, DestructorCancelsPendingCreate)
{
    auto env = std::make_unique<server_env>(m_thread_pool.get(), make_pipe_name());
    auto future = env->create_pipe(0);

    env.reset();
    wait_for_result(future, [] {}, "The create canceled by server env destruction");

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::canceled);
    EXPECT_FALSE(result.endpoint);
}

TEST_F(WinPipeServerEnv, TimeoutOfOneCreateDoesNotCancelAnother)
{
    auto pipe_name = make_pipe_name();
    server_env env(m_thread_pool.get(), pipe_name);
    auto pending = env.create_pipe(0);
    auto timed = env.create_pipe(0, immediate_timeout);

    wait_for_result(timed,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The independently timed out create");
    auto timed_result = get_endpoint_result(timed);
    ASSERT_EQ(timed_result.state, pipe_wait_res::timeout);
    ASSERT_FALSE(timed_result.endpoint);

    auto client = open_client(pipe_name);
    if(client.empty()) {
        ADD_FAILURE() << "CreateFileW did not connect to the remaining server pipe";
        env.cancel_all_pending_server_endpoints();
    }
    wait_for_result(pending,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The create unaffected by another timeout");
    auto pending_result = get_endpoint_result(pending);
    EXPECT_EQ(pending_result.state, pipe_wait_res::success);
    EXPECT_TRUE(pending_result.endpoint);
}

TEST_F(WinPipeServerEnv, SupportsMultipleConcurrentCreateOperations)
{
    auto pipe_name = make_pipe_name();
    server_env env(m_thread_pool.get(), pipe_name);
    auto first = env.create_pipe(0);
    auto second = env.create_pipe(0);

    auto first_client = open_client(pipe_name);
    auto second_client = open_client(pipe_name);
    if(first_client.empty() || second_client.empty()) {
        ADD_FAILURE() << "CreateFileW did not connect both client handles";
        env.cancel_all_pending_server_endpoints();
    }
    wait_for_result(first, [&] { env.cancel_all_pending_server_endpoints(); }, "The first concurrent create");
    wait_for_result(second, [&] { env.cancel_all_pending_server_endpoints(); }, "The second concurrent create");

    auto first_result = get_endpoint_result(first);
    auto second_result = get_endpoint_result(second);
    EXPECT_EQ(first_result.state, pipe_wait_res::success);
    EXPECT_EQ(second_result.state, pipe_wait_res::success);
    EXPECT_TRUE(first_result.endpoint);
    EXPECT_TRUE(second_result.endpoint);
    EXPECT_NE(first_result.endpoint, second_result.endpoint);
}

TEST_F(WinPipeServerEnv, InvalidPipeNameFailsCreate)
{
    server_env env(m_thread_pool.get(), std::wstring(512, L'x'));

    auto future = env.create_pipe(0);
    wait_for_result(future,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The invalid-name server endpoint create operation");

    auto result = get_endpoint_result(future);
    EXPECT_EQ(result.state, pipe_wait_res::failed);
    EXPECT_FALSE(result.endpoint);
}

TEST_F(WinPipeServerEnv, CancelsOnlyPendingCreateOperationsWithSpecifiedClientId)
{
    constexpr auto canceled_client_id = 101u;
    constexpr auto remaining_client_id = 202u;
    auto pipe_name = make_pipe_name();
    server_env env(m_thread_pool.get(), pipe_name);
    auto first_canceled = env.create_pipe(canceled_client_id);
    auto second_canceled = env.create_pipe(canceled_client_id);
    auto remaining = env.create_pipe(remaining_client_id);

    env.cancel_pending_server_endpoints(canceled_client_id);

    wait_for_result(first_canceled,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The first create canceled by client id");
    wait_for_result(second_canceled,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The second create canceled by client id");
    auto first_result = get_endpoint_result(first_canceled);
    auto second_result = get_endpoint_result(second_canceled);
    ASSERT_EQ(first_result.state, pipe_wait_res::canceled);
    ASSERT_EQ(second_result.state, pipe_wait_res::canceled);
    ASSERT_FALSE(first_result.endpoint);
    ASSERT_FALSE(second_result.endpoint);

    auto client = open_client(pipe_name);
    if(client.empty()) {
        ADD_FAILURE() << "CreateFileW did not connect to the remaining server pipe";
        env.cancel_all_pending_server_endpoints();
    }
    wait_for_result(remaining,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The create belonging to another client id");
    auto remaining_result = get_endpoint_result(remaining);
    EXPECT_EQ(remaining_result.state, pipe_wait_res::success);
    EXPECT_TRUE(remaining_result.endpoint);
}

TEST_F(WinPipeServerEnv, CancelsAllPendingCreateOperationsWithDifferentClientIds)
{
    server_env env(m_thread_pool.get(), make_pipe_name());
    auto first = env.create_pipe(101);
    auto second = env.create_pipe(202);
    auto third = env.create_pipe(303);

    env.cancel_all_pending_server_endpoints();

    wait_for_result(first,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The first create canceled without filtering");
    wait_for_result(second,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The second create canceled without filtering");
    wait_for_result(third,
                    [&] { env.cancel_all_pending_server_endpoints(); },
                    "The third create canceled without filtering");
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
#endif
