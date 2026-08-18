#ifdef _WIN32
#include <rpc-lib/pipe/win-pipe/win-pipe-operations/win-pipe-read-operation.h>
#include <rpc-lib/consts.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

using namespace vshalygin;
using namespace vshalygin::cl;
using namespace vshalygin::rpc;
using namespace vshalygin::rpc::internal;
using namespace vshalygin::win;
using namespace testing;

namespace {
    constexpr DWORD completion_timeout_ms = 5000;
    constexpr DWORD no_completion_timeout_ms = 100;
    constexpr ULONG_PTR completion_key = 0xC0DEC0DEu;
    constexpr size_t initial_buffer_size = 8192;

    struct connected_pipe
    {
        std::shared_ptr<value_locker<pipe_handle>> reader;
        pipe_handle writer;
    };

    struct completion_packet
    {
        bool success = false;
        DWORD bytes_transferred = 0;
        ULONG_PTR key = 0;
        OVERLAPPED *overlapped = nullptr;
        DWORD error = ERROR_SUCCESS;
    };

    struct read_operation_result
    {
        win_pipe_operation_res state = win_pipe_operation_res::unknown;
        buffer data;
    };
}

class WinPipeReadOperation
    : public Test
{
protected:
    using operation = win_pipe_read_operation;

    void SetUp() override
    {
        m_thread_pool = std::make_unique<thread_pool>(1);
    }

    void TearDown() override
    {
        m_thread_pool->stop();
    }

    std::shared_ptr<operation> create_operation(
        const std::shared_ptr<value_locker<pipe_handle>> &pipe)
    {
        return operation::create(pipe, m_thread_pool.get());
    }

    std::shared_ptr<operation> create_operation()
    {
        return create_operation(std::make_shared<value_locker<pipe_handle>>());
    }

    static std::wstring make_pipe_name()
    {
        static std::atomic<std::uint64_t> next_id = 0;

        return L"\\\\.\\pipe\\rpc-lib-test-read-operation-"
            + std::to_wstring(::GetCurrentProcessId())
            + L"-"
            + std::to_wstring(next_id.fetch_add(1, std::memory_order_relaxed));
    }

    static connected_pipe create_connected_pipe()
    {
        auto pipe_name = make_pipe_name();
        auto pipe_buffer_size = static_cast<DWORD>(MaxTransferMessageSize * 2u);

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

        connected_pipe result;
        result.reader = std::make_shared<value_locker<pipe_handle>>(std::move(server));
        result.writer = std::move(client);
        return result;
    }

    static HANDLE get_pipe_handle(const connected_pipe &pipe)
    {
        return pipe.reader->lock()->get();
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
            result[i] = static_cast<std::byte>((i * 31u + 17u) % 251u);
        }
        return result;
    }

    static bool write_message(const pipe_handle &pipe, const buffer &message)
    {
        DWORD bytes_written = 0;
        auto success = ::WriteFile(pipe.get(),
                                   message.data(),
                                   static_cast<DWORD>(message.size()),
                                   &bytes_written,
                                   nullptr) != FALSE;
        EXPECT_TRUE(success) << "WriteFile failed, error=" << ::GetLastError();
        EXPECT_EQ(bytes_written, message.size());
        return success && bytes_written == message.size();
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
            ADD_FAILURE() << "The read operation did not complete in time, error=" << packet.error;

            // The test invokes ReadFile and CancelIo on the same thread. Closing the peer
            // additionally guarantees completion if a partial message made cancel a no-op.
            op->set_canceled_if_possible();
            op->cancel();
            pipe.writer.reset();
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
                                   const iocp_handle &iocp,
                                   connected_pipe &pipe,
                                   completion_packet packet)
    {
        while(true) {
            expect_own_completion(op, packet);
            op->add_read_bytes(packet.bytes_transferred);

            if(packet.success) {
                op->set_success();
                op->resolve();
                return;
            }

            if(packet.error != ERROR_MORE_DATA) {
                if(packet.error != ERROR_OPERATION_ABORTED) {
                    op->set_failed_if_possible();
                }
                op->resolve();
                return;
            }

            if(!op->add_buffer_chunk()) {
                op->set_failed_if_possible();
                op->resolve();
                return;
            }

            auto ec = start(op);
            if(ec) {
                ADD_FAILURE() << "ReadFile failed while reading the rest of a message, error="
                              << ec.value();
                op->set_failed_if_possible();
                op->resolve();
                return;
            }

            packet = wait_for_completion(op, iocp, pipe);
        }
    }

    static read_operation_result get_result(rpc::future<rpc::ftuple<win_pipe_operation_res, buffer>> &future)
    {
        read_operation_result result;
        future.get().apply([&](win_pipe_operation_res state, buffer &&data) {
            result.state = state;
            result.data = std::move(data);
        });
        return result;
    }

    static read_operation_result resolve(const std::shared_ptr<operation> &op)
    {
        auto future = op->get_future();
        op->resolve();
        return get_result(future);
    }

protected:
    std::unique_ptr<thread_pool> m_thread_pool;
};

TEST_F(WinPipeReadOperation, ReadsPendingMessageThroughIocp)
{
    auto pipe = create_connected_pipe();
    auto iocp = associate_with_iocp(pipe);
    auto op = create_operation(pipe.reader);
    auto future = op->get_future();
    auto expected = make_message(257);

    ASSERT_FALSE(start(op));
    ASSERT_TRUE(write_message(pipe.writer, expected));

    auto packet = wait_for_completion(op, iocp, pipe);
    EXPECT_TRUE(packet.success);
    EXPECT_EQ(packet.error, static_cast<DWORD>(ERROR_SUCCESS));
    EXPECT_EQ(packet.bytes_transferred, expected.size());
    finish_from_packet(op, iocp, pipe, packet);

    auto result = get_result(future);
    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_EQ(result.data, expected);
}

TEST_F(WinPipeReadOperation, ReadsMessageThatWasAvailableBeforeReadFile)
{
    auto pipe = create_connected_pipe();
    auto iocp = associate_with_iocp(pipe);
    auto op = create_operation(pipe.reader);
    auto future = op->get_future();
    auto expected = make_message(1024);
    ASSERT_TRUE(write_message(pipe.writer, expected));

    ASSERT_FALSE(start(op));
    auto packet = wait_for_completion(op, iocp, pipe);
    EXPECT_TRUE(packet.success);
    EXPECT_EQ(packet.bytes_transferred, expected.size());
    finish_from_packet(op, iocp, pipe, packet);

    auto result = get_result(future);
    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_EQ(result.data, expected);
}

TEST_F(WinPipeReadOperation, ReadsPendingMessageAcrossMultipleBuffers)
{
    auto pipe = create_connected_pipe();
    auto iocp = associate_with_iocp(pipe);
    auto op = create_operation(pipe.reader);
    auto future = op->get_future();
    auto expected = make_message(initial_buffer_size * 7 + 113);

    ASSERT_FALSE(start(op));
    ASSERT_TRUE(write_message(pipe.writer, expected));

    auto packet = wait_for_completion(op, iocp, pipe);
    EXPECT_FALSE(packet.success);
    EXPECT_EQ(packet.error, static_cast<DWORD>(ERROR_MORE_DATA));
    EXPECT_EQ(packet.bytes_transferred, initial_buffer_size);
    finish_from_packet(op, iocp, pipe, packet);

    auto result = get_result(future);
    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_EQ(result.data, expected);
}

TEST_F(WinPipeReadOperation, ReadsPreexistingMessageAcrossMultipleBuffers)
{
    auto pipe = create_connected_pipe();
    auto iocp = associate_with_iocp(pipe);
    auto op = create_operation(pipe.reader);
    auto future = op->get_future();
    auto expected = make_message(initial_buffer_size * 7 + 113);
    ASSERT_TRUE(write_message(pipe.writer, expected));

    ASSERT_FALSE(start(op));
    auto packet = wait_for_completion(op, iocp, pipe);
    EXPECT_TRUE(packet.success || packet.error == ERROR_MORE_DATA)
        << "Unexpected read error=" << packet.error;
    finish_from_packet(op, iocp, pipe, packet);

    auto result = get_result(future);
    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_EQ(result.data, expected);
}

TEST_F(WinPipeReadOperation, CancelPendingRead)
{
    auto pipe = create_connected_pipe();
    auto iocp = associate_with_iocp(pipe);
    auto op = create_operation(pipe.reader);
    auto future = op->get_future();

    ASSERT_FALSE(start(op));
    op->set_canceled_if_possible();
    op->cancel();

    auto packet = wait_for_completion(op, iocp, pipe);
    EXPECT_FALSE(packet.success);
    EXPECT_EQ(packet.error, static_cast<DWORD>(ERROR_OPERATION_ABORTED));
    finish_from_packet(op, iocp, pipe, packet);

    auto result = get_result(future);
    EXPECT_EQ(result.state, win_pipe_operation_res::canceled);
    EXPECT_EQ(result.data.size(), 0u);
}

TEST_F(WinPipeReadOperation, TimeoutPendingRead)
{
    auto pipe = create_connected_pipe();
    auto iocp = associate_with_iocp(pipe);
    auto op = create_operation(pipe.reader);
    auto future = op->get_future();

    ASSERT_FALSE(start(op));
    op->set_timeout_if_possible();
    op->cancel();

    auto packet = wait_for_completion(op, iocp, pipe);
    EXPECT_FALSE(packet.success);
    EXPECT_EQ(packet.error, static_cast<DWORD>(ERROR_OPERATION_ABORTED));
    finish_from_packet(op, iocp, pipe, packet);

    auto result = get_result(future);
    EXPECT_EQ(result.state, win_pipe_operation_res::timeout);
    EXPECT_EQ(result.data.size(), 0u);
}

TEST_F(WinPipeReadOperation, CancelDoesNotAbortReadAfterBytesWereAccounted)
{
    auto pipe = create_connected_pipe();
    auto iocp = associate_with_iocp(pipe);
    auto op = create_operation(pipe.reader);
    auto future = op->get_future();
    auto expected = make_message(31);

    // This is the state in which a previous ERROR_MORE_DATA completion has already
    // contributed bytes and another ReadFile is pending.
    op->add_read_bytes(1);
    ASSERT_FALSE(start(op));
    op->set_canceled_if_possible();
    op->cancel();

    auto absent_packet = wait_for_completion(iocp, no_completion_timeout_ms);
    EXPECT_EQ(absent_packet.overlapped, nullptr);
    EXPECT_EQ(absent_packet.error, static_cast<DWORD>(WAIT_TIMEOUT));

    ASSERT_TRUE(write_message(pipe.writer, expected));
    auto packet = wait_for_completion(op, iocp, pipe);
    EXPECT_TRUE(packet.success);
    finish_from_packet(op, iocp, pipe, packet);

    auto result = get_result(future);
    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_EQ(result.data.size(), expected.size() + 1);
}

TEST_F(WinPipeReadOperation, PeerDisconnectFailsPendingRead)
{
    auto pipe = create_connected_pipe();
    auto iocp = associate_with_iocp(pipe);
    auto op = create_operation(pipe.reader);
    auto future = op->get_future();

    ASSERT_FALSE(start(op));
    pipe.writer.reset();

    auto packet = wait_for_completion(op, iocp, pipe);
    EXPECT_FALSE(packet.success);
    EXPECT_TRUE(packet.error == ERROR_BROKEN_PIPE || packet.error == ERROR_PIPE_NOT_CONNECTED)
        << "Unexpected pipe disconnect error=" << packet.error;
    finish_from_packet(op, iocp, pipe, packet);

    auto result = get_result(future);
    EXPECT_EQ(result.state, win_pipe_operation_res::failed);
    EXPECT_EQ(result.data.size(), 0u);
}

TEST_F(WinPipeReadOperation, StartOnInvalidHandleReportsError)
{
    auto op = create_operation();

    auto ec = start(op);
    EXPECT_EQ(ec.value(), static_cast<int>(ERROR_INVALID_HANDLE));
    EXPECT_EQ(ec.category(), std::system_category());

    op->set_failed_if_possible();
    auto result = resolve(op);
    EXPECT_EQ(result.state, win_pipe_operation_res::failed);
    EXPECT_EQ(result.data.size(), 0u);
}

TEST_F(WinPipeReadOperation, BufferGrowthStopsAtMaximumMessageSize)
{
    auto op = create_operation();
    size_t chunks_added = 0;

    while(op->add_buffer_chunk()) {
        ++chunks_added;
    }

    EXPECT_EQ(chunks_added, 7u);
    EXPECT_FALSE(op->add_buffer_chunk());

    op->add_read_bytes(MaxTransferMessageSize);
    op->set_success();
    auto result = resolve(op);
    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_EQ(result.data.size(), static_cast<size_t>(MaxTransferMessageSize));
}

TEST_F(WinPipeReadOperation, ReadByteAccountingSupportsConcurrentCompletions)
{
    auto op = create_operation();
    std::vector<std::thread> workers;
    constexpr size_t worker_count = 8;
    constexpr DWORD bytes_per_worker = 1000;

    for(size_t i = 0; i < worker_count; ++i) {
        workers.emplace_back([op] { op->add_read_bytes(bytes_per_worker); });
    }
    for(auto &worker : workers) {
        worker.join();
    }

    op->set_success();
    auto result = resolve(op);
    EXPECT_EQ(result.state, win_pipe_operation_res::success);
    EXPECT_EQ(result.data.size(), worker_count * bytes_per_worker);
}

TEST_F(WinPipeReadOperation, ConditionalTerminalStatesDoNotReplaceOneAnother)
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

TEST_F(WinPipeReadOperation, FailureOverridesTimeout)
{
    auto op = create_operation();

    op->set_timeout_if_possible();
    op->set_failed_if_possible();

    EXPECT_EQ(op->get_result(), win_pipe_operation_res::failed);
}

TEST_F(WinPipeReadOperation, SuccessCanReplaceEarlierTerminalState)
{
    auto op = create_operation();

    op->set_canceled_if_possible();
    op->set_success();
    op->set_timeout_if_possible();
    op->set_failed_if_possible();

    EXPECT_EQ(op->get_result(), win_pipe_operation_res::success);
}
#endif
