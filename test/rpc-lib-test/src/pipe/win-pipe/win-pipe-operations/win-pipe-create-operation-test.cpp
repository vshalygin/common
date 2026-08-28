#ifdef _WIN32
#include <rpc-lib/pipe/win-pipe/win-pipe-operations/win-pipe-create-operation.h>

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <system_error>

using namespace vshalygin::cl;
using namespace vshalygin::rpc::internal;
using namespace vshalygin::win;
using namespace testing;

namespace {
    constexpr DWORD completion_timeout_ms = 5000;
    constexpr ULONG_PTR completion_key = 0xC0DEC0DEu;

    struct resolved_operation
    {
        win_pipe_operation_res state = win_pipe_operation_res::unknown;
        pipe_handle pipe;
    };

    struct completion_packet
    {
        bool success = false;
        DWORD bytes_transferred = 0;
        ULONG_PTR key = 0;
        OVERLAPPED *overlapped = nullptr;
        DWORD error = ERROR_SUCCESS;
    };
}

class WinPipeCreateOperation
    : public Test
{
protected:
    using operation = win_pipe_create_operation;

    void SetUp() override
    {
        m_thread_pool = std::make_unique<thread_pool>(1);
    }

    void TearDown() override
    {
        m_thread_pool->stop();
    }

    std::shared_ptr<operation> create_operation()
    {
        return operation::create(make_pipe_name(), m_thread_pool.get());
    }

    std::shared_ptr<operation> create_operation(const std::wstring &pipe_name)
    {
        return operation::create(pipe_name, m_thread_pool.get());
    }

    static std::wstring make_pipe_name()
    {
        static std::atomic<std::uint64_t> next_id = 0;

        return L"rpc-lib-test-create-operation-"
            + std::to_wstring(::GetCurrentProcessId())
            + L"-"
            + std::to_wstring(next_id.fetch_add(1, std::memory_order_relaxed));
    }

    static std::wstring make_full_pipe_name(const std::wstring &pipe_name)
    {
        return L"\\\\.\\pipe\\" + pipe_name;
    }

    static HANDLE get_pipe_handle(const std::shared_ptr<operation> &op)
    {
        HANDLE handle = INVALID_HANDLE_VALUE;
        op->exec([&](HANDLE value) { handle = value; });
        return handle;
    }

    static pipe_handle open_client(const std::wstring &pipe_name)
    {
        return pipe_handle(::CreateFileW(make_full_pipe_name(pipe_name).c_str(),
                                         GENERIC_READ | GENERIC_WRITE,
                                         0,
                                         nullptr,
                                         OPEN_EXISTING,
                                         0,
                                         nullptr));
    }

    static iocp_handle associate_with_iocp(const std::shared_ptr<operation> &op)
    {
        iocp_handle iocp(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1));
        if(iocp.empty()) {
            throw std::system_error(::GetLastError(), std::system_category());
        }

        if(::CreateIoCompletionPort(get_pipe_handle(op), iocp.get(), completion_key, 0) == nullptr) {
            throw std::system_error(::GetLastError(), std::system_category());
        }

        return iocp;
    }

    static completion_packet wait_for_completion(const std::shared_ptr<operation> &op,
                                                 const iocp_handle &iocp)
    {
        completion_packet packet;
        packet.success = ::GetQueuedCompletionStatus(iocp.get(),
                                                      &packet.bytes_transferred,
                                                      &packet.key,
                                                      &packet.overlapped,
                                                      completion_timeout_ms) != FALSE;
        if(!packet.success) {
            packet.error = ::GetLastError();
        }

        if(packet.overlapped == nullptr) {
            ADD_FAILURE() << "The connect operation did not complete in time, error=" << packet.error;

            // ConnectNamedPipe uses the current test thread, therefore CancelIo from this
            // thread can safely terminate it before the operation object is destroyed.
            op->cancel(false);
            packet.success = ::GetQueuedCompletionStatus(iocp.get(),
                                                          &packet.bytes_transferred,
                                                          &packet.key,
                                                          &packet.overlapped,
                                                          INFINITE) != FALSE;
            packet.error = packet.success ? ERROR_SUCCESS : ::GetLastError();
        }

        return packet;
    }

    static resolved_operation resolve(const std::shared_ptr<operation> &op)
    {
        auto future = op->get_future();
        op->resolve();

        resolved_operation result;
        future.get().lock().with([&](win_pipe_operation_res state, pipe_handle &&pipe) {
            result.state = state;
            result.pipe = std::move(pipe);
        });
        return result;
    }

protected:
    std::unique_ptr<thread_pool> m_thread_pool;
};

TEST_F(WinPipeCreateOperation, CreatePipeAndResolveSuccessTransfersHandle)
{
    auto op = create_operation();

    EXPECT_EQ(get_pipe_handle(op), INVALID_HANDLE_VALUE);
    ASSERT_TRUE(op->create_pipe());

    auto raw_pipe = get_pipe_handle(op);
    ASSERT_NE(raw_pipe, INVALID_HANDLE_VALUE);

    DWORD flags = 0;
    EXPECT_TRUE(::GetNamedPipeInfo(raw_pipe, &flags, nullptr, nullptr, nullptr));
    EXPECT_NE(flags & PIPE_SERVER_END, 0u);

    op->set_success();
    auto result = resolve(op);

    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_FALSE(result.pipe.empty());
    EXPECT_EQ(result.pipe.get(), raw_pipe);
    EXPECT_EQ(get_pipe_handle(op), INVALID_HANDLE_VALUE);
}

TEST_F(WinPipeCreateOperation, CreatePipeFailsForTooLongName)
{
    auto op = create_operation(std::wstring(512, L'x'));

    EXPECT_FALSE(op->create_pipe());
    auto result = resolve(op);

    EXPECT_EQ(result.state, win_pipe_operation_res::failed);
    EXPECT_TRUE(result.pipe.empty());
    EXPECT_EQ(get_pipe_handle(op), INVALID_HANDLE_VALUE);
}

TEST_F(WinPipeCreateOperation, ConnectCompletesAsynchronouslyThroughIocp)
{
    auto pipe_name = make_pipe_name();
    auto op = create_operation(pipe_name);
    ASSERT_TRUE(op->create_pipe());
    auto iocp = associate_with_iocp(op);

    ASSERT_TRUE(op->start_wait_connect());

    auto client = open_client(pipe_name);
    if(client.empty()) {
        ADD_FAILURE() << "CreateFileW failed, error=" << ::GetLastError();
        op->cancel(false);
    }

    auto packet = wait_for_completion(op, iocp);
    EXPECT_TRUE(packet.success);
    EXPECT_EQ(packet.error, static_cast<DWORD>(ERROR_SUCCESS));
    EXPECT_EQ(packet.key, completion_key);
    EXPECT_NE(packet.overlapped, nullptr);

    op->set_success();
    auto result = resolve(op);

    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_FALSE(result.pipe.empty());
}

TEST_F(WinPipeCreateOperation, ClientConnectedBeforeConnectNamedPipeCompletesSynchronously)
{
    auto pipe_name = make_pipe_name();
    auto op = create_operation(pipe_name);
    ASSERT_TRUE(op->create_pipe());
    auto iocp = associate_with_iocp(op);
    auto client = open_client(pipe_name);
    ASSERT_FALSE(client.empty()) << "CreateFileW failed, error=" << ::GetLastError();

    auto waiting = op->start_wait_connect();
    EXPECT_FALSE(waiting);
    if(waiting) {
        auto packet = wait_for_completion(op, iocp);
        EXPECT_TRUE(packet.success);
        op->set_success();
    }

    auto result = resolve(op);
    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_FALSE(result.pipe.empty());
}

TEST_F(WinPipeCreateOperation, ClientDisconnectedBeforeConnectNamedPipeFails)
{
    auto pipe_name = make_pipe_name();
    auto op = create_operation(pipe_name);
    ASSERT_TRUE(op->create_pipe());
    auto iocp = associate_with_iocp(op);

    auto client = open_client(pipe_name);
    ASSERT_FALSE(client.empty()) << "CreateFileW failed, error=" << ::GetLastError();
    client.reset();

    auto waiting = op->start_wait_connect();
    EXPECT_FALSE(waiting);
    if(waiting) {
        op->cancel(false);
        auto packet = wait_for_completion(op, iocp);
        EXPECT_EQ(packet.error, static_cast<DWORD>(ERROR_OPERATION_ABORTED));
    }

    auto result = resolve(op);
    EXPECT_EQ(result.state, win_pipe_operation_res::failed);
    EXPECT_TRUE(result.pipe.empty());
    EXPECT_EQ(get_pipe_handle(op), INVALID_HANDLE_VALUE);
}

TEST_F(WinPipeCreateOperation, CancelPendingConnect)
{
    auto op = create_operation();
    ASSERT_TRUE(op->create_pipe());
    auto iocp = associate_with_iocp(op);
    ASSERT_TRUE(op->start_wait_connect());

    op->cancel(false);
    auto packet = wait_for_completion(op, iocp);

    EXPECT_FALSE(packet.success);
    EXPECT_EQ(packet.error, static_cast<DWORD>(ERROR_OPERATION_ABORTED));
    EXPECT_EQ(packet.key, completion_key);
    EXPECT_NE(packet.overlapped, nullptr);

    auto result = resolve(op);
    EXPECT_EQ(result.state, win_pipe_operation_res::canceled);
    EXPECT_TRUE(result.pipe.empty());
}

TEST_F(WinPipeCreateOperation, TimeoutPendingConnect)
{
    auto op = create_operation();
    ASSERT_TRUE(op->create_pipe());
    auto iocp = associate_with_iocp(op);
    ASSERT_TRUE(op->start_wait_connect());

    op->cancel(true);
    auto packet = wait_for_completion(op, iocp);

    EXPECT_FALSE(packet.success);
    EXPECT_EQ(packet.error, static_cast<DWORD>(ERROR_OPERATION_ABORTED));
    EXPECT_EQ(packet.key, completion_key);
    EXPECT_NE(packet.overlapped, nullptr);

    auto result = resolve(op);
    EXPECT_EQ(result.state, win_pipe_operation_res::timeout);
    EXPECT_TRUE(result.pipe.empty());
}

TEST_F(WinPipeCreateOperation, CancelBeforePipeCreationDoesNotAccessInvalidHandle)
{
    auto op = create_operation();

    op->cancel(false);
    auto result = resolve(op);

    EXPECT_EQ(result.state, win_pipe_operation_res::canceled);
    EXPECT_TRUE(result.pipe.empty());
}

TEST_F(WinPipeCreateOperation, TerminalStateSettersDoNotReplaceSuccess)
{
    auto op = create_operation();
    ASSERT_TRUE(op->create_pipe());

    op->set_success();
    op->set_canceled_if_possible();
    op->set_timeout_if_possible();
    op->set_failed_if_possible();

    auto result = resolve(op);
    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_FALSE(result.pipe.empty());
}

TEST_F(WinPipeCreateOperation, SuccessCanReplaceCancellation)
{
    auto op = create_operation();
    ASSERT_TRUE(op->create_pipe());

    op->cancel(false);
    op->set_success();

    auto result = resolve(op);
    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_FALSE(result.pipe.empty());
}
#endif
