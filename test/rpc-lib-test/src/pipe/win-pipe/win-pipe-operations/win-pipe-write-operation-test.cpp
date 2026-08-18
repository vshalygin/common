#ifdef _WIN32
#include <rpc-lib/pipe/win-pipe/win-pipe-operations/win-pipe-write-operation.h>
#include <rpc-lib/consts.h>

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <system_error>

using namespace vshalygin::cl;
using namespace vshalygin::rpc;
using namespace vshalygin::rpc::internal;
using namespace vshalygin::win;
using namespace testing;

namespace {
    constexpr DWORD completion_timeout_ms = 5000;
    constexpr ULONG_PTR completion_key = 0xC0DEC0DEu;
    constexpr DWORD pipe_buffer_size = 4096;

    struct connected_pipe
    {
        std::shared_ptr<value_locker<pipe_handle>> writer;
        pipe_handle reader;
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

class WinPipeWriteOperation
    : public Test
{
protected:
    using operation = win_pipe_write_operation;

    void SetUp() override
    {
        m_thread_pool = std::make_unique<thread_pool>(1);
    }

    void TearDown() override
    {
        m_thread_pool->stop();
    }

    std::shared_ptr<operation> create_operation(
        const std::shared_ptr<value_locker<pipe_handle>> &pipe,
        buffer &&message)
    {
        return operation::create(pipe, std::move(message), m_thread_pool.get());
    }

    std::shared_ptr<operation> create_operation()
    {
        return create_operation(std::make_shared<value_locker<pipe_handle>>(), buffer{});
    }

    static std::wstring make_pipe_name()
    {
        static std::atomic<std::uint64_t> next_id = 0;

        return L"\\\\.\\pipe\\rpc-lib-test-write-operation-"
            + std::to_wstring(::GetCurrentProcessId())
            + L"-"
            + std::to_wstring(next_id.fetch_add(1, std::memory_order_relaxed));
    }

    static connected_pipe create_connected_pipe()
    {
        auto pipe_name = make_pipe_name();
        pipe_handle server(::CreateNamedPipeW(pipe_name.c_str(),
                                              PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                              PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                              1,
                                              pipe_buffer_size,
                                              pipe_buffer_size,
                                              0,
                                              nullptr));
        if(server.empty()) {
            throw std::system_error(::GetLastError(), std::system_category());
        }

        pipe_handle client(::CreateFileW(pipe_name.c_str(),
                                         GENERIC_READ | GENERIC_WRITE,
                                         0,
                                         nullptr,
                                         OPEN_EXISTING,
                                         0,
                                         nullptr));
        if(client.empty()) {
            throw std::system_error(::GetLastError(), std::system_category());
        }

        DWORD read_mode = PIPE_READMODE_MESSAGE;
        if(!::SetNamedPipeHandleState(client.get(), &read_mode, nullptr, nullptr)) {
            throw std::system_error(::GetLastError(), std::system_category());
        }

        connected_pipe result;
        result.writer = std::make_shared<value_locker<pipe_handle>>(std::move(server));
        result.reader = std::move(client);
        return result;
    }

    static HANDLE get_pipe_handle(const connected_pipe &pipe)
    {
        return pipe.writer->lock()->get();
    }

    static iocp_handle associate_with_iocp(const connected_pipe &pipe)
    {
        iocp_handle iocp(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1));
        if(iocp.empty()) {
            throw std::system_error(::GetLastError(), std::system_category());
        }

        if(::CreateIoCompletionPort(get_pipe_handle(pipe), iocp.get(), completion_key, 0) == nullptr) {
            throw std::system_error(::GetLastError(), std::system_category());
        }

        return iocp;
    }

    static buffer make_message(size_t size)
    {
        buffer result(size);
        for(size_t i = 0; i < result.size(); ++i) {
            result[i] = static_cast<std::byte>((i * 37u + 11u) % 251u);
        }
        return result;
    }

    static bool read_message(connected_pipe &pipe, buffer &message)
    {
        DWORD bytes_read = 0;
        auto success = ::ReadFile(pipe.reader.get(),
                                  message.data(),
                                  static_cast<DWORD>(message.size()),
                                  &bytes_read,
                                  nullptr) != FALSE;
        if(!success) {
            ADD_FAILURE() << "ReadFile failed, error=" << ::GetLastError();

            // If the write is still pending, disconnecting the reader guarantees an
            // IOCP completion before the operation object is destroyed.
            pipe.reader.reset();
            return false;
        }

        EXPECT_EQ(bytes_read, message.size());
        return bytes_read == message.size();
    }

    static completion_packet wait_for_completion(const iocp_handle &iocp, DWORD timeout_ms)
    {
        completion_packet packet;
        packet.success = ::GetQueuedCompletionStatus(iocp.get(),
                                                      &packet.bytes_transferred,
                                                      &packet.key,
                                                      &packet.overlapped,
                                                      timeout_ms) != FALSE;
        if(!packet.success) {
            packet.error = ::GetLastError();
        }
        return packet;
    }

    static completion_packet wait_for_completion(const std::shared_ptr<operation> &op,
                                                 const iocp_handle &iocp,
                                                 connected_pipe &pipe)
    {
        auto packet = wait_for_completion(iocp, completion_timeout_ms);
        if(packet.overlapped == nullptr) {
            ADD_FAILURE() << "The write operation did not complete in time, error=" << packet.error;

            // WriteFile and CancelIo are invoked by the same test thread.
            op->set_canceled_if_possible();
            op->cancel();
            pipe.reader.reset();
            packet = wait_for_completion(iocp, INFINITE);
        }

        return packet;
    }

    static void expect_own_completion(const std::shared_ptr<operation> &op,
                                      const completion_packet &packet)
    {
        EXPECT_EQ(packet.key, completion_key);
        EXPECT_EQ(packet.overlapped, reinterpret_cast<OVERLAPPED *>(op.get()));
    }

    static std::error_code start(const std::shared_ptr<operation> &op)
    {
        std::error_code ec;
        op->start(ec);
        return ec;
    }

    static void finish_from_packet(const std::shared_ptr<operation> &op,
                                   const completion_packet &packet)
    {
        expect_own_completion(op, packet);
        if(packet.success) {
            op->set_success();
        } else if(packet.error != ERROR_OPERATION_ABORTED) {
            op->set_failed_if_possible();
        }
        op->resolve();
    }

    static win_pipe_operation_res get_result(
        vshalygin::rpc::future<win_pipe_operation_res> &future)
    {
        auto result = win_pipe_operation_res::unknown;
        future.get().apply([&](win_pipe_operation_res state) { result = state; });
        return result;
    }

    static win_pipe_operation_res resolve(const std::shared_ptr<operation> &op)
    {
        auto future = op->get_future();
        op->resolve();
        return get_result(future);
    }

protected:
    std::unique_ptr<thread_pool> m_thread_pool;
};

TEST_F(WinPipeWriteOperation, WritesMessageAndKeepsMovedBufferAlive)
{
    auto pipe = create_connected_pipe();
    auto iocp = associate_with_iocp(pipe);
    auto message = make_message(257);
    auto expected = message.copy();
    auto received = buffer(expected.size());
    auto op = create_operation(pipe.writer, std::move(message));
    auto future = op->get_future();

    EXPECT_EQ(message.size(), 0u);
    ASSERT_FALSE(start(op));
    EXPECT_TRUE(read_message(pipe, received));

    auto packet = wait_for_completion(op, iocp, pipe);
    EXPECT_TRUE(packet.success);
    EXPECT_EQ(packet.error, static_cast<DWORD>(ERROR_SUCCESS));
    EXPECT_EQ(packet.bytes_transferred, expected.size());
    finish_from_packet(op, packet);

    EXPECT_EQ(get_result(future), win_pipe_operation_res::success);
    EXPECT_EQ(received, expected);
}

TEST_F(WinPipeWriteOperation, CompletesLargePendingWriteWhenPeerReads)
{
    auto pipe = create_connected_pipe();
    auto iocp = associate_with_iocp(pipe);
    auto message = make_message(MaxTransferMessageSize);
    auto expected = message.copy();
    auto received = buffer(expected.size());
    auto op = create_operation(pipe.writer, std::move(message));
    auto future = op->get_future();

    ASSERT_FALSE(start(op));
    auto absent_packet = wait_for_completion(iocp, 0);
    ASSERT_EQ(absent_packet.overlapped, nullptr)
        << "The test write unexpectedly completed before the reader was started";
    EXPECT_EQ(absent_packet.error, static_cast<DWORD>(WAIT_TIMEOUT));

    EXPECT_TRUE(read_message(pipe, received));
    auto packet = wait_for_completion(op, iocp, pipe);
    EXPECT_TRUE(packet.success);
    EXPECT_EQ(packet.bytes_transferred, expected.size());
    finish_from_packet(op, packet);

    EXPECT_EQ(get_result(future), win_pipe_operation_res::success);
    EXPECT_EQ(received, expected);
}

TEST_F(WinPipeWriteOperation, CancelPendingWrite)
{
    auto pipe = create_connected_pipe();
    auto iocp = associate_with_iocp(pipe);
    auto message = make_message(MaxTransferMessageSize);
    auto op = create_operation(pipe.writer, std::move(message));
    auto future = op->get_future();

    ASSERT_FALSE(start(op));
    auto absent_packet = wait_for_completion(iocp, 0);
    ASSERT_EQ(absent_packet.overlapped, nullptr)
        << "The test write unexpectedly completed before cancellation";

    op->set_canceled_if_possible();
    op->cancel();
    auto packet = wait_for_completion(op, iocp, pipe);
    EXPECT_FALSE(packet.success);
    EXPECT_EQ(packet.error, static_cast<DWORD>(ERROR_OPERATION_ABORTED));
    finish_from_packet(op, packet);

    EXPECT_EQ(get_result(future), win_pipe_operation_res::canceled);
}

TEST_F(WinPipeWriteOperation, TimeoutPendingWrite)
{
    auto pipe = create_connected_pipe();
    auto iocp = associate_with_iocp(pipe);
    auto message = make_message(MaxTransferMessageSize);
    auto op = create_operation(pipe.writer, std::move(message));
    auto future = op->get_future();

    ASSERT_FALSE(start(op));
    auto absent_packet = wait_for_completion(iocp, 0);
    ASSERT_EQ(absent_packet.overlapped, nullptr)
        << "The test write unexpectedly completed before timeout";

    op->set_timeout_if_possible();
    op->cancel();
    auto packet = wait_for_completion(op, iocp, pipe);
    EXPECT_FALSE(packet.success);
    EXPECT_EQ(packet.error, static_cast<DWORD>(ERROR_OPERATION_ABORTED));
    finish_from_packet(op, packet);

    EXPECT_EQ(get_result(future), win_pipe_operation_res::timeout);
}

TEST_F(WinPipeWriteOperation, PeerDisconnectFailsPendingWrite)
{
    auto pipe = create_connected_pipe();
    auto iocp = associate_with_iocp(pipe);
    auto message = make_message(MaxTransferMessageSize);
    auto op = create_operation(pipe.writer, std::move(message));
    auto future = op->get_future();

    ASSERT_FALSE(start(op));
    auto absent_packet = wait_for_completion(iocp, 0);
    ASSERT_EQ(absent_packet.overlapped, nullptr)
        << "The test write unexpectedly completed before disconnect";

    pipe.reader.reset();
    auto packet = wait_for_completion(op, iocp, pipe);
    EXPECT_FALSE(packet.success);
    EXPECT_TRUE(packet.error == ERROR_BROKEN_PIPE
                || packet.error == ERROR_NO_DATA
                || packet.error == ERROR_PIPE_NOT_CONNECTED)
        << "Unexpected pipe disconnect error=" << packet.error;
    finish_from_packet(op, packet);

    EXPECT_EQ(get_result(future), win_pipe_operation_res::failed);
}

TEST_F(WinPipeWriteOperation, StartOnInvalidHandleReportsError)
{
    auto op = create_operation();

    auto ec = start(op);
    EXPECT_EQ(ec.value(), static_cast<int>(ERROR_INVALID_HANDLE));
    EXPECT_EQ(ec.category(), std::system_category());

    op->set_failed_if_possible();
    EXPECT_EQ(resolve(op), win_pipe_operation_res::failed);
}

TEST_F(WinPipeWriteOperation, ConditionalTerminalStatesDoNotReplaceOneAnother)
{
    {
        auto op = create_operation();
        op->set_canceled_if_possible();
        op->set_timeout_if_possible();
        op->set_failed_if_possible();
        EXPECT_EQ(op->get_result(), win_pipe_operation_res::canceled);
    }

    {
        auto op = create_operation();
        op->set_failed_if_possible();
        op->set_canceled_if_possible();
        op->set_timeout_if_possible();
        EXPECT_EQ(op->get_result(), win_pipe_operation_res::failed);
    }
}

TEST_F(WinPipeWriteOperation, FailureOverridesTimeout)
{
    auto op = create_operation();

    op->set_timeout_if_possible();
    op->set_failed_if_possible();

    EXPECT_EQ(op->get_result(), win_pipe_operation_res::failed);
}

TEST_F(WinPipeWriteOperation, SuccessCanReplaceEarlierTerminalState)
{
    auto op = create_operation();

    op->set_canceled_if_possible();
    op->set_success();
    op->set_timeout_if_possible();
    op->set_failed_if_possible();

    EXPECT_EQ(op->get_result(), win_pipe_operation_res::success);
}
#endif
