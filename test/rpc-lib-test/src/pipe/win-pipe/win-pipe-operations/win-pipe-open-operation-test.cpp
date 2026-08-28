#ifdef _WIN32
#include <rpc-lib/pipe/win-pipe/win-pipe-operations/win-pipe-open-operation.h>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

using namespace vshalygin::cl;
using namespace vshalygin::rpc::internal;
using namespace vshalygin::win;
using namespace testing;

namespace {
    constexpr auto operation_timeout = std::chrono::seconds(5);
    constexpr auto retry_observation_timeout = std::chrono::milliseconds(250);

    struct open_operation_result
    {
        win_pipe_operation_res state = win_pipe_operation_res::unknown;
        pipe_handle pipe;
    };
}

class WinPipeOpenOperation
    : public Test
{
protected:
    using operation = win_pipe_open_operation;

    void SetUp() override
    {
        m_thread_pool = std::make_unique<thread_pool>(1);
    }

    void TearDown() override
    {
        m_thread_pool->stop();
    }

    static std::wstring make_pipe_name()
    {
        static std::atomic<std::uint64_t> next_id = 0;

        return L"rpc-lib-test-open-operation-"
            + std::to_wstring(::GetCurrentProcessId())
            + L"-"
            + std::to_wstring(next_id.fetch_add(1, std::memory_order_relaxed));
    }

    static std::wstring make_full_pipe_name(const std::wstring &pipe_name)
    {
        return L"\\\\.\\pipe\\" + pipe_name;
    }

    static pipe_handle create_server(const std::wstring &pipe_name,
                                     DWORD pipe_mode = PIPE_TYPE_MESSAGE
                                                     | PIPE_READMODE_MESSAGE
                                                     | PIPE_WAIT)
    {
        return pipe_handle(::CreateNamedPipeW(make_full_pipe_name(pipe_name).c_str(),
                                              PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                              pipe_mode,
                                              1,
                                              8192,
                                              8192,
                                              0,
                                              nullptr));
    }

    static pipe_handle open_occupying_client(const std::wstring &pipe_name)
    {
        return pipe_handle(::CreateFileW(make_full_pipe_name(pipe_name).c_str(),
                                         GENERIC_READ | GENERIC_WRITE,
                                         0,
                                         nullptr,
                                         OPEN_EXISTING,
                                         FILE_FLAG_OVERLAPPED,
                                         nullptr));
    }

    static open_operation_result get_result(operation::future &future)
    {
        open_operation_result result;
        future.get().lock().with([&](win_pipe_operation_res state, pipe_handle &&pipe) {
            result.state = state;
            result.pipe = std::move(pipe);
        });
        return result;
    }

    static open_operation_result wait_for_result(operation &op,
                                                 operation::future &future)
    {
        if(!future.wait_for(operation_timeout)) {
            ADD_FAILURE() << "The pipe open operation did not complete in time";
            op.cancel(false);
        }

        return get_result(future);
    }

protected:
    std::unique_ptr<thread_pool> m_thread_pool;
};

TEST_F(WinPipeOpenOperation, OpensMessagePipeAndSetsMessageReadMode)
{
    auto pipe_name = make_pipe_name();
    auto server = create_server(pipe_name);
    ASSERT_FALSE(server.empty()) << "CreateNamedPipeW failed, error=" << ::GetLastError();

    operation op(pipe_name, m_thread_pool.get());
    auto future = op.get_future();
    op.start();

    auto result = wait_for_result(op, future);
    ASSERT_EQ(result.state, win_pipe_operation_res::success);
    ASSERT_FALSE(result.pipe.empty());

    DWORD state = 0;
    ASSERT_TRUE(::GetNamedPipeHandleStateW(result.pipe.get(),
                                           &state,
                                           nullptr,
                                           nullptr,
                                           nullptr,
                                           nullptr,
                                           0));
    EXPECT_NE(state & PIPE_READMODE_MESSAGE, 0u);
}

TEST_F(WinPipeOpenOperation, RetriesUntilPipeAppears)
{
    auto pipe_name = make_pipe_name();
    operation op(pipe_name, m_thread_pool.get());
    auto future = op.get_future();
    op.start();

    EXPECT_FALSE(future.wait_for(retry_observation_timeout));

    auto server = create_server(pipe_name);
    ASSERT_FALSE(server.empty()) << "CreateNamedPipeW failed, error=" << ::GetLastError();

    auto result = wait_for_result(op, future);
    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_FALSE(result.pipe.empty());
}

TEST_F(WinPipeOpenOperation, RetriesWhileAllPipeInstancesAreBusy)
{
    auto pipe_name = make_pipe_name();
    auto server = create_server(pipe_name);
    ASSERT_FALSE(server.empty()) << "CreateNamedPipeW failed, error=" << ::GetLastError();

    auto occupying_client = open_occupying_client(pipe_name);
    ASSERT_FALSE(occupying_client.empty()) << "CreateFileW failed, error=" << ::GetLastError();

    operation op(pipe_name, m_thread_pool.get());
    auto future = op.get_future();
    op.start();

    EXPECT_FALSE(future.wait_for(retry_observation_timeout));

    occupying_client.reset();
    server.reset();
    auto replacement_server = create_server(pipe_name);
    ASSERT_FALSE(replacement_server.empty()) << "CreateNamedPipeW failed, error=" << ::GetLastError();

    auto result = wait_for_result(op, future);
    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_FALSE(result.pipe.empty());
}

TEST_F(WinPipeOpenOperation, FailsForTooLongPipeName)
{
    operation op(std::wstring(512, L'x'), m_thread_pool.get());
    auto future = op.get_future();
    op.start();

    auto result = wait_for_result(op, future);
    EXPECT_EQ(result.state, win_pipe_operation_res::failed);
    EXPECT_TRUE(result.pipe.empty());
}

TEST_F(WinPipeOpenOperation, FailsToSetMessageReadModeForBytePipe)
{
    auto pipe_name = make_pipe_name();
    auto server = create_server(pipe_name, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT);
    ASSERT_FALSE(server.empty()) << "CreateNamedPipeW failed, error=" << ::GetLastError();

    operation op(pipe_name, m_thread_pool.get());
    auto future = op.get_future();
    op.start();

    auto result = wait_for_result(op, future);
    EXPECT_EQ(result.state, win_pipe_operation_res::failed);
    EXPECT_FALSE(result.pipe.empty());
}

TEST_F(WinPipeOpenOperation, CancelsPendingOpen)
{
    operation op(make_pipe_name(), m_thread_pool.get());
    auto future = op.get_future();
    op.start();

    op.cancel(false);
    auto result = wait_for_result(op, future);

    EXPECT_EQ(result.state, win_pipe_operation_res::canceled);
    EXPECT_TRUE(result.pipe.empty());
}

TEST_F(WinPipeOpenOperation, TimesOutPendingOpen)
{
    operation op(make_pipe_name(), m_thread_pool.get());
    auto future = op.get_future();
    op.start();

    op.cancel(true);
    auto result = wait_for_result(op, future);

    EXPECT_EQ(result.state, win_pipe_operation_res::timeout);
    EXPECT_TRUE(result.pipe.empty());
}

TEST_F(WinPipeOpenOperation, FirstCancellationReasonWins)
{
    {
        operation op(make_pipe_name(), m_thread_pool.get());
        auto future = op.get_future();
        op.cancel(false);
        op.cancel(true);
        op.start();

        auto result = wait_for_result(op, future);
        EXPECT_EQ(result.state, win_pipe_operation_res::canceled);
        EXPECT_TRUE(result.pipe.empty());
    }

    {
        operation op(make_pipe_name(), m_thread_pool.get());
        auto future = op.get_future();
        op.cancel(true);
        op.cancel(false);
        op.start();

        auto result = wait_for_result(op, future);
        EXPECT_EQ(result.state, win_pipe_operation_res::timeout);
        EXPECT_TRUE(result.pipe.empty());
    }
}

TEST_F(WinPipeOpenOperation, CancellationAfterSuccessfulOpenDoesNotReplaceResult)
{
    auto pipe_name = make_pipe_name();
    auto server = create_server(pipe_name);
    ASSERT_FALSE(server.empty()) << "CreateNamedPipeW failed, error=" << ::GetLastError();

    operation op(pipe_name, m_thread_pool.get());
    auto future = op.get_future();
    op.start();
    ASSERT_TRUE(future.wait_for(operation_timeout));

    op.cancel(true);
    auto result = get_result(future);

    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_FALSE(result.pipe.empty());
}

TEST_F(WinPipeOpenOperation, DestructorCancelsPendingOpenAndJoinsThread)
{
    operation::future future;
    {
        operation op(make_pipe_name(), m_thread_pool.get());
        future = op.get_future();
        op.start();
    }

    ASSERT_TRUE(future.wait_for(operation_timeout));
    auto result = get_result(future);
    EXPECT_EQ(result.state, win_pipe_operation_res::canceled);
    EXPECT_TRUE(result.pipe.empty());
}

TEST_F(WinPipeOpenOperation, CanBeDestroyedBeforeStart)
{
    operation op(make_pipe_name(), m_thread_pool.get());
}
#endif
