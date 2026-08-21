#include <rpc-lib/pipe/tcp-pipe/tcp-pipe-endpoint.h>
#include <rpc-lib/consts.h>

#include <common-lib/synchronization/event.h>

#include <boost/asio/error.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/endian/conversion.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <utility>

using namespace vshalygin::cl;
using namespace vshalygin::rpc;
using namespace testing;

namespace {
    using tcp = boost::asio::ip::tcp;

    constexpr auto operation_timeout = std::chrono::seconds(5);
    constexpr auto immediate_timeout = std::chrono::milliseconds(0);
    constexpr auto long_timeout = std::chrono::seconds(30);
    constexpr auto partial_transfer_delay = std::chrono::milliseconds(25);
    constexpr auto socket_poll_delay = std::chrono::milliseconds(1);

    struct endpoint_read_result
    {
        pipe_op_res state = pipe_op_res::failed;
        buffer data;
    };

    struct connected_sockets
    {
        connected_sockets(tcp::socket &&first_, tcp::socket &&second_)
            : first(std::move(first_))
            , second(std::move(second_))
        {}

        tcp::socket first;
        tcp::socket second;
    };

    struct endpoint_pair
    {
        std::unique_ptr<tcp_pipe_endpoint> first;
        std::unique_ptr<tcp_pipe_endpoint> second;
    };

    struct raw_endpoint_pair
    {
        raw_endpoint_pair(std::unique_ptr<tcp_pipe_endpoint> endpoint_, tcp::socket &&peer_)
            : endpoint(std::move(endpoint_))
            , peer(std::move(peer_))
        {}

        std::unique_ptr<tcp_pipe_endpoint> endpoint;
        tcp::socket peer;
    };

    struct blocked_raw_endpoint_pair
    {
        blocked_raw_endpoint_pair(std::unique_ptr<tcp_pipe_endpoint> endpoint_,
                                  tcp::socket &&peer_,
                                  size_t filler_size_)
            : endpoint(std::move(endpoint_))
            , peer(std::move(peer_))
            , filler_size(filler_size_)
        {}

        std::unique_ptr<tcp_pipe_endpoint> endpoint;
        tcp::socket peer;
        size_t filler_size = 0;
    };

    struct raw_frame_result
    {
        bool completed = false;
        buffer data;
    };
}

class TcpPipeEndpoint
    : public Test
{
protected:
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
            future.wait();
        }
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

    static void wait_for_read(read_future &future,
                              tcp_pipe_endpoint &endpoint,
                              const char *operation_name)
    {
        wait_for_result(future, [&] { endpoint.invalidate(); }, operation_name);
    }

    static void wait_for_write(write_future &future,
                               tcp_pipe_endpoint &endpoint,
                               const char *operation_name)
    {
        wait_for_result(future, [&] { endpoint.invalidate(); }, operation_name);
    }

    connected_sockets create_connected_sockets(bool constrain_buffers = false)
    {
        auto &io_context = m_thread_pool->get_io_context();
        tcp::acceptor acceptor(io_context,
                               tcp::endpoint(boost::asio::ip::make_address_v4("127.0.0.1"), 0));

        tcp::socket first(io_context);
        if(constrain_buffers) {
            acceptor.set_option(boost::asio::socket_base::receive_buffer_size(1024));
            first.open(tcp::v4());
            first.set_option(boost::asio::socket_base::send_buffer_size(1024));
        }
        first.connect(acceptor.local_endpoint());

        tcp::socket second(io_context);
        acceptor.accept(second);

        if(constrain_buffers) {
            second.set_option(boost::asio::socket_base::receive_buffer_size(1024));
        }

        first.set_option(tcp::no_delay(true));
        second.set_option(tcp::no_delay(true));
        return connected_sockets(std::move(first), std::move(second));
    }

    endpoint_pair create_endpoints()
    {
        auto sockets = create_connected_sockets();
        endpoint_pair result;
        result.first = std::make_unique<tcp_pipe_endpoint>(m_thread_pool,
                                                          std::move(sockets.first));
        result.second = std::make_unique<tcp_pipe_endpoint>(m_thread_pool,
                                                           std::move(sockets.second));
        return result;
    }

    raw_endpoint_pair create_raw_endpoint()
    {
        auto sockets = create_connected_sockets();
        auto endpoint = std::make_unique<tcp_pipe_endpoint>(m_thread_pool,
                                                           std::move(sockets.first));
        return raw_endpoint_pair(std::move(endpoint), std::move(sockets.second));
    }

    raw_endpoint_pair create_constrained_raw_endpoint()
    {
        auto sockets = create_connected_sockets(true);
        auto endpoint = std::make_unique<tcp_pipe_endpoint>(m_thread_pool,
                                                           std::move(sockets.first));
        return raw_endpoint_pair(std::move(endpoint), std::move(sockets.second));
    }

    static bool is_would_block(const boost::system::error_code &ec)
    {
        return ec == boost::asio::error::would_block ||
               ec == boost::asio::error::try_again;
    }

    static bool set_non_blocking(tcp::socket &socket)
    {
        boost::system::error_code ec;
        socket.non_blocking(true, ec);
        if(ec) {
            ADD_FAILURE() << "Failed to make a raw loopback socket non-blocking: " << ec.message();
            return false;
        }
        return true;
    }

    static bool write_exact(tcp::socket &socket, const void *data, size_t size)
    {
        if(!set_non_blocking(socket)) {
            return false;
        }

        auto bytes = static_cast<const std::byte *>(data);
        size_t offset = 0;
        const auto deadline = std::chrono::steady_clock::now() + operation_timeout;
        while(offset != size && std::chrono::steady_clock::now() < deadline) {
            boost::system::error_code ec;
            const auto transferred = socket.write_some(
                boost::asio::buffer(bytes + offset, size - offset), ec);
            if(!ec) {
                offset += transferred;
            } else if(is_would_block(ec)) {
                std::this_thread::sleep_for(socket_poll_delay);
            } else {
                ADD_FAILURE() << "Raw loopback write failed: " << ec.message();
                return false;
            }
        }

        if(offset != size) {
            ADD_FAILURE() << "Raw loopback write did not complete in time";
            return false;
        }
        return true;
    }

    static bool read_exact(tcp::socket &socket, void *data, size_t size)
    {
        if(!set_non_blocking(socket)) {
            return false;
        }

        auto bytes = static_cast<std::byte *>(data);
        size_t offset = 0;
        const auto deadline = std::chrono::steady_clock::now() + operation_timeout;
        while(offset != size && std::chrono::steady_clock::now() < deadline) {
            boost::system::error_code ec;
            const auto transferred = socket.read_some(
                boost::asio::buffer(bytes + offset, size - offset), ec);
            if(!ec) {
                offset += transferred;
            } else if(is_would_block(ec)) {
                std::this_thread::sleep_for(socket_poll_delay);
            } else {
                ADD_FAILURE() << "Raw loopback read failed: " << ec.message();
                return false;
            }
        }

        if(offset != size) {
            ADD_FAILURE() << "Raw loopback read did not complete in time";
            return false;
        }
        return true;
    }

    static bool discard_exact(tcp::socket &socket, size_t size)
    {
        if(!set_non_blocking(socket)) {
            return false;
        }

        std::array<std::byte, 4096> scratch;
        size_t discarded = 0;
        const auto deadline = std::chrono::steady_clock::now() + operation_timeout;
        while(discarded != size && std::chrono::steady_clock::now() < deadline) {
            const auto chunk_size = std::min(scratch.size(), size - discarded);
            boost::system::error_code ec;
            const auto transferred = socket.read_some(
                boost::asio::buffer(scratch.data(), chunk_size), ec);
            if(!ec) {
                discarded += transferred;
            } else if(is_would_block(ec)) {
                std::this_thread::sleep_for(socket_poll_delay);
            } else {
                ADD_FAILURE() << "Failed while discarding loopback data: " << ec.message();
                return false;
            }
        }

        if(discarded != size) {
            ADD_FAILURE() << "Loopback data was not discarded in time";
            return false;
        }
        return true;
    }

    static bool write_frame(tcp::socket &socket, const buffer &message)
    {
        const auto header = boost::endian::native_to_big(static_cast<uint32_t>(message.size()));
        return write_exact(socket, &header, sizeof(header)) &&
               write_exact(socket, message.data(), message.size());
    }

    static bool write_header(tcp::socket &socket, uint32_t payload_size)
    {
        const auto header = boost::endian::native_to_big(payload_size);
        return write_exact(socket, &header, sizeof(header));
    }

    static raw_frame_result read_frame(tcp::socket &socket)
    {
        raw_frame_result result;
        uint32_t header = 0;
        if(!read_exact(socket, &header, sizeof(header))) {
            return result;
        }

        const auto payload_size = boost::endian::big_to_native(header);
        if(payload_size == 0 || payload_size > MaxTransferMessageSize) {
            ADD_FAILURE() << "The endpoint emitted an invalid frame size: " << payload_size;
            return result;
        }

        result.data = buffer(payload_size);
        if(!read_exact(socket, result.data.data(), result.data.size())) {
            return result;
        }
        result.completed = true;
        return result;
    }

    static size_t fill_send_buffer(tcp::socket &socket)
    {
        socket.set_option(boost::asio::socket_base::send_buffer_size(1024));
        if(!set_non_blocking(socket)) {
            return 0;
        }

        std::array<std::byte, 16384> filler{};
        size_t total = 0;
        bool was_blocked = false;
        constexpr size_t max_filler_size = 16u * 1024u * 1024u;
        const auto deadline = std::chrono::steady_clock::now() + operation_timeout;
        while(total < max_filler_size && std::chrono::steady_clock::now() < deadline) {
            boost::system::error_code ec;
            const auto transferred = socket.write_some(boost::asio::buffer(filler), ec);
            if(!ec) {
                if(transferred == 0) {
                    ADD_FAILURE() << "A non-empty loopback write transferred zero bytes";
                    break;
                }
                total += transferred;
            } else if(is_would_block(ec)) {
                was_blocked = true;
                break;
            } else {
                ADD_FAILURE() << "Failed while filling the loopback send buffer: " << ec.message();
                break;
            }
        }

        boost::system::error_code ec;
        socket.non_blocking(false, ec);
        if(ec) {
            ADD_FAILURE() << "Failed to restore blocking socket mode: " << ec.message();
        }
        if(!was_blocked) {
            ADD_FAILURE() << "The loopback send buffer could not be filled";
        }
        return total;
    }

    blocked_raw_endpoint_pair create_blocked_raw_endpoint()
    {
        auto sockets = create_connected_sockets(true);
        const auto filler_size = fill_send_buffer(sockets.first);
        auto endpoint = std::make_unique<tcp_pipe_endpoint>(m_thread_pool,
                                                           std::move(sockets.first));
        return blocked_raw_endpoint_pair(std::move(endpoint),
                                         std::move(sockets.second),
                                         filler_size);
    }

protected:
    std::shared_ptr<thread_pool> m_thread_pool;
};

TEST_F(TcpPipeEndpoint, IsConnectedAfterCreation)
{
    auto endpoints = create_endpoints();

    EXPECT_TRUE(endpoints.first->is_connected());
    EXPECT_TRUE(endpoints.second->is_connected());
}

TEST_F(TcpPipeEndpoint, TransfersMessagesSimultaneouslyInBothDirections)
{
    auto endpoints = create_endpoints();
    auto first_message = make_message(8192u * 3u + 17u, 1);
    auto second_message = make_message(4096u * 5u + 29u, 2);
    auto expected_first = first_message.copy();
    auto expected_second = second_message.copy();

    auto first_read = endpoints.first->read_async();
    auto second_read = endpoints.second->read_async();
    auto first_write = endpoints.first->write_async(std::move(first_message));
    auto second_write = endpoints.second->write_async(std::move(second_message));

    wait_for_read(first_read, *endpoints.first, "The first bidirectional read");
    wait_for_read(second_read, *endpoints.second, "The second bidirectional read");
    wait_for_write(first_write, *endpoints.first, "The first bidirectional write");
    wait_for_write(second_write, *endpoints.second, "The second bidirectional write");

    auto first_result = get_read_result(first_read);
    auto second_result = get_read_result(second_read);
    EXPECT_EQ(get_write_result(first_write), pipe_op_res::success);
    EXPECT_EQ(get_write_result(second_write), pipe_op_res::success);
    EXPECT_EQ(first_result.state, pipe_op_res::success);
    EXPECT_EQ(second_result.state, pipe_op_res::success);
    EXPECT_EQ(first_result.data, expected_second);
    EXPECT_EQ(second_result.data, expected_first);
}

TEST_F(TcpPipeEndpoint, TransfersMaximumSizeMessage)
{
    auto endpoints = create_endpoints();
    auto message = make_message(MaxTransferMessageSize, 3);
    auto expected = message.copy();

    auto read = endpoints.second->read_async();
    auto write = endpoints.first->write_async(std::move(message));
    wait_for_read(read, *endpoints.second, "The maximum-size TCP read");
    wait_for_write(write, *endpoints.first, "The maximum-size TCP write");

    auto result = get_read_result(read);
    EXPECT_EQ(get_write_result(write), pipe_op_res::success);
    EXPECT_EQ(result.state, pipe_op_res::success);
    EXPECT_EQ(result.data, expected);
}

TEST_F(TcpPipeEndpoint, PreservesOrderOfQueuedReadsAndWrites)
{
    auto endpoints = create_endpoints();
    auto first_message = make_message(101, 1);
    auto second_message = make_message(203, 2);
    auto third_message = make_message(307, 3);
    auto expected_first = first_message.copy();
    auto expected_second = second_message.copy();
    auto expected_third = third_message.copy();

    auto first_read = endpoints.second->read_async();
    auto second_read = endpoints.second->read_async();
    auto third_read = endpoints.second->read_async();
    auto first_write = endpoints.first->write_async(std::move(first_message));
    auto second_write = endpoints.first->write_async(std::move(second_message));
    auto third_write = endpoints.first->write_async(std::move(third_message));

    wait_for_read(first_read, *endpoints.second, "The first queued TCP read");
    wait_for_read(second_read, *endpoints.second, "The second queued TCP read");
    wait_for_read(third_read, *endpoints.second, "The third queued TCP read");
    wait_for_write(first_write, *endpoints.first, "The first queued TCP write");
    wait_for_write(second_write, *endpoints.first, "The second queued TCP write");
    wait_for_write(third_write, *endpoints.first, "The third queued TCP write");

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

TEST_F(TcpPipeEndpoint, ReadsHeaderAndPayloadDeliveredInParts)
{
    auto endpoints = create_raw_endpoint();
    auto message = make_message(257, 7);
    auto read = endpoints.endpoint->read_async();
    const auto header = boost::endian::native_to_big(static_cast<uint32_t>(message.size()));
    const auto header_bytes = reinterpret_cast<const std::byte *>(&header);

    ASSERT_TRUE(write_exact(endpoints.peer, header_bytes, 1));
    std::this_thread::sleep_for(partial_transfer_delay);
    ASSERT_TRUE(write_exact(endpoints.peer, header_bytes + 1, sizeof(header) - 1));
    std::this_thread::sleep_for(partial_transfer_delay);
    ASSERT_TRUE(write_exact(endpoints.peer, message.data(), 3));
    std::this_thread::sleep_for(partial_transfer_delay);
    ASSERT_TRUE(write_exact(endpoints.peer, message.data() + 3, message.size() - 3));

    wait_for_read(read, *endpoints.endpoint, "The fragmented TCP read");
    auto result = get_read_result(read);
    EXPECT_EQ(result.state, pipe_op_res::success);
    EXPECT_EQ(result.data, message);
}

TEST_F(TcpPipeEndpoint, RejectsZeroLengthFrame)
{
    auto endpoints = create_raw_endpoint();
    auto callback_event = std::make_shared<event>();
    endpoints.endpoint->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(), [callback_event] { callback_event->set(); }));
    auto read = endpoints.endpoint->read_async();

    ASSERT_TRUE(write_header(endpoints.peer, 0));
    wait_for_read(read, *endpoints.endpoint, "The zero-length frame read");

    auto result = get_read_result(read);
    EXPECT_EQ(result.state, pipe_op_res::failed);
    EXPECT_EQ(result.data.size(), 0u);
    EXPECT_FALSE(endpoints.endpoint->is_connected());
    EXPECT_TRUE(callback_event->wait_for(operation_timeout));
}

TEST_F(TcpPipeEndpoint, RejectsOversizedFrameBeforeReadingPayload)
{
    auto endpoints = create_raw_endpoint();
    auto read = endpoints.endpoint->read_async();

    ASSERT_TRUE(write_header(endpoints.peer, MaxTransferMessageSize + 1u));
    wait_for_read(read, *endpoints.endpoint, "The oversized frame read");

    auto result = get_read_result(read);
    EXPECT_EQ(result.state, pipe_op_res::failed);
    EXPECT_EQ(result.data.size(), 0u);
    EXPECT_FALSE(endpoints.endpoint->is_connected());
}

TEST_F(TcpPipeEndpoint, TimedReadAndWriteCanCompleteSuccessfully)
{
    auto endpoints = create_endpoints();
    auto message = make_message(4097, 8);
    auto expected = message.copy();

    auto read = endpoints.second->read_async(long_timeout);
    auto write = endpoints.first->write_async(std::move(message), long_timeout);
    wait_for_read(read, *endpoints.second, "The successful timed TCP read");
    wait_for_write(write, *endpoints.first, "The successful timed TCP write");

    auto result = get_read_result(read);
    EXPECT_EQ(get_write_result(write), pipe_op_res::success);
    EXPECT_EQ(result.state, pipe_op_res::success);
    EXPECT_EQ(result.data, expected);
    EXPECT_TRUE(endpoints.first->is_connected());
    EXPECT_TRUE(endpoints.second->is_connected());
}

TEST_F(TcpPipeEndpoint, ActiveReadTimeoutDrainsFrameAndPreservesNextMessage)
{
    auto endpoints = create_raw_endpoint();
    auto discarded_message = make_message(127, 1);
    auto next_message = make_message(239, 2);

    auto timed_read = endpoints.endpoint->read_async(immediate_timeout);
    wait_for_read(timed_read, *endpoints.endpoint, "The active timed TCP read");
    auto timed_result = get_read_result(timed_read);
    ASSERT_EQ(timed_result.state, pipe_op_res::timeout);
    EXPECT_EQ(timed_result.data.size(), 0u);

    auto next_read = endpoints.endpoint->read_async();
    ASSERT_TRUE(write_frame(endpoints.peer, discarded_message));
    ASSERT_TRUE(write_frame(endpoints.peer, next_message));
    wait_for_read(next_read, *endpoints.endpoint, "The read following an active timeout");

    auto next_result = get_read_result(next_read);
    EXPECT_EQ(next_result.state, pipe_op_res::success);
    EXPECT_EQ(next_result.data, next_message);
    EXPECT_TRUE(endpoints.endpoint->is_connected());
}

TEST_F(TcpPipeEndpoint, ReadTimeoutAfterPartialHeaderPreservesFraming)
{
    auto endpoints = create_raw_endpoint();
    auto discarded_message = make_message(83, 13);
    auto next_message = make_message(179, 14);
    const auto header = boost::endian::native_to_big(
        static_cast<uint32_t>(discarded_message.size()));
    const auto header_bytes = reinterpret_cast<const std::byte *>(&header);

    auto timed_read = endpoints.endpoint->read_async(std::chrono::milliseconds(100));
    ASSERT_TRUE(write_exact(endpoints.peer, header_bytes, 1));
    std::this_thread::sleep_for(partial_transfer_delay);
    wait_for_read(timed_read, *endpoints.endpoint, "The TCP read timing out after a partial header");
    ASSERT_EQ(get_read_result(timed_read).state, pipe_op_res::timeout);

    auto next_read = endpoints.endpoint->read_async();
    ASSERT_TRUE(write_exact(endpoints.peer, header_bytes + 1, sizeof(header) - 1));
    ASSERT_TRUE(write_exact(endpoints.peer,
                            discarded_message.data(),
                            discarded_message.size()));
    ASSERT_TRUE(write_frame(endpoints.peer, next_message));
    wait_for_read(next_read, *endpoints.endpoint, "The read following a partial-header timeout");

    auto result = get_read_result(next_read);
    EXPECT_EQ(result.state, pipe_op_res::success);
    EXPECT_EQ(result.data, next_message);
    EXPECT_TRUE(endpoints.endpoint->is_connected());
}

TEST_F(TcpPipeEndpoint, ReadTimeoutAfterPartialPayloadPreservesFraming)
{
    auto endpoints = create_raw_endpoint();
    auto discarded_message = make_message(271, 15);
    auto next_message = make_message(149, 16);
    const auto header = boost::endian::native_to_big(
        static_cast<uint32_t>(discarded_message.size()));
    constexpr size_t first_payload_part_size = 7;

    auto timed_read = endpoints.endpoint->read_async(std::chrono::milliseconds(100));
    ASSERT_TRUE(write_exact(endpoints.peer, &header, sizeof(header)));
    ASSERT_TRUE(write_exact(endpoints.peer,
                            discarded_message.data(),
                            first_payload_part_size));
    std::this_thread::sleep_for(partial_transfer_delay);
    wait_for_read(timed_read, *endpoints.endpoint, "The TCP read timing out after a partial payload");
    ASSERT_EQ(get_read_result(timed_read).state, pipe_op_res::timeout);

    auto next_read = endpoints.endpoint->read_async();
    ASSERT_TRUE(write_exact(endpoints.peer,
                            discarded_message.data() + first_payload_part_size,
                            discarded_message.size() - first_payload_part_size));
    ASSERT_TRUE(write_frame(endpoints.peer, next_message));
    wait_for_read(next_read, *endpoints.endpoint, "The read following a partial-payload timeout");

    auto result = get_read_result(next_read);
    EXPECT_EQ(result.state, pipe_op_res::success);
    EXPECT_EQ(result.data, next_message);
    EXPECT_TRUE(endpoints.endpoint->is_connected());
}

TEST_F(TcpPipeEndpoint, QueuedReadTimeoutDoesNotConsumeMessage)
{
    auto endpoints = create_raw_endpoint();
    auto first_message = make_message(113, 1);
    auto second_message = make_message(227, 2);

    auto active_read = endpoints.endpoint->read_async();
    auto queued_read = endpoints.endpoint->read_async(immediate_timeout);
    wait_for_read(queued_read, *endpoints.endpoint, "The queued timed TCP read");
    EXPECT_EQ(get_read_result(queued_read).state, pipe_op_res::timeout);

    ASSERT_TRUE(write_frame(endpoints.peer, first_message));
    wait_for_read(active_read, *endpoints.endpoint, "The active read preceding a queued timeout");
    auto active_result = get_read_result(active_read);
    EXPECT_EQ(active_result.state, pipe_op_res::success);
    EXPECT_EQ(active_result.data, first_message);

    auto next_read = endpoints.endpoint->read_async();
    ASSERT_TRUE(write_frame(endpoints.peer, second_message));
    wait_for_read(next_read, *endpoints.endpoint, "The read following a queued timeout");
    auto next_result = get_read_result(next_read);
    EXPECT_EQ(next_result.state, pipe_op_res::success);
    EXPECT_EQ(next_result.data, second_message);
}

TEST_F(TcpPipeEndpoint, ActiveWriteTimeoutFinishesPhysicalFrameAndPreservesNextMessage)
{
    auto endpoints = create_blocked_raw_endpoint();
    ASSERT_NE(endpoints.filler_size, 0u);
    auto timed_message = make_message(MaxTransferMessageSize, 4);
    auto expected_timed_message = timed_message.copy();

    auto timed_write = endpoints.endpoint->write_async(std::move(timed_message), immediate_timeout);
    wait_for_write(timed_write, *endpoints.endpoint, "The active timed TCP write");
    ASSERT_EQ(get_write_result(timed_write), pipe_op_res::timeout);
    EXPECT_TRUE(endpoints.endpoint->is_connected());

    ASSERT_TRUE(discard_exact(endpoints.peer, endpoints.filler_size));
    auto timed_frame = read_frame(endpoints.peer);
    ASSERT_TRUE(timed_frame.completed);
    EXPECT_EQ(timed_frame.data, expected_timed_message);

    auto next_message = make_message(193, 5);
    auto expected_next_message = next_message.copy();
    auto next_write = endpoints.endpoint->write_async(std::move(next_message));
    auto next_frame = read_frame(endpoints.peer);
    wait_for_write(next_write, *endpoints.endpoint, "The write following an active timeout");
    ASSERT_TRUE(next_frame.completed);
    EXPECT_EQ(get_write_result(next_write), pipe_op_res::success);
    EXPECT_EQ(next_frame.data, expected_next_message);
}

TEST_F(TcpPipeEndpoint, QueuedWriteTimeoutDoesNotPutFrameOnWire)
{
    auto endpoints = create_blocked_raw_endpoint();
    ASSERT_NE(endpoints.filler_size, 0u);
    auto active_message = make_message(MaxTransferMessageSize, 6);
    auto expected_active_message = active_message.copy();
    auto discarded_message = make_message(311, 7);

    auto active_write = endpoints.endpoint->write_async(std::move(active_message));
    auto queued_write = endpoints.endpoint->write_async(std::move(discarded_message), immediate_timeout);
    wait_for_write(queued_write, *endpoints.endpoint, "The queued timed TCP write");
    ASSERT_EQ(get_write_result(queued_write), pipe_op_res::timeout);

    ASSERT_TRUE(discard_exact(endpoints.peer, endpoints.filler_size));
    auto active_frame = read_frame(endpoints.peer);
    wait_for_write(active_write, *endpoints.endpoint, "The active write preceding a queued timeout");
    ASSERT_TRUE(active_frame.completed);
    EXPECT_EQ(get_write_result(active_write), pipe_op_res::success);
    EXPECT_EQ(active_frame.data, expected_active_message);

    auto final_message = make_message(197, 8);
    auto expected_final_message = final_message.copy();
    auto final_write = endpoints.endpoint->write_async(std::move(final_message));
    auto final_frame = read_frame(endpoints.peer);
    wait_for_write(final_write, *endpoints.endpoint, "The write following a queued timeout");
    ASSERT_TRUE(final_frame.completed);
    EXPECT_EQ(get_write_result(final_write), pipe_op_res::success);
    EXPECT_EQ(final_frame.data, expected_final_message);
}

TEST_F(TcpPipeEndpoint, InvalidateCancelsActiveAndQueuedOperationsAndIsIdempotent)
{
    auto endpoints = create_blocked_raw_endpoint();
    ASSERT_NE(endpoints.filler_size, 0u);
    auto callback_event = std::make_shared<event>();
    auto callback_count = std::make_shared<std::atomic<unsigned>>(0);
    endpoints.endpoint->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(),
        [callback_event, callback_count] {
            callback_count->fetch_add(1, std::memory_order_relaxed);
            callback_event->set();
        }));

    auto first_read = endpoints.endpoint->read_async(long_timeout);
    auto second_read = endpoints.endpoint->read_async(long_timeout);
    auto first_write = endpoints.endpoint->write_async(make_message(MaxTransferMessageSize, 9), long_timeout);
    auto second_write = endpoints.endpoint->write_async(make_message(67, 10), long_timeout);

    endpoints.endpoint->invalidate();
    wait_for_read(first_read, *endpoints.endpoint, "The first invalidated TCP read");
    wait_for_read(second_read, *endpoints.endpoint, "The second invalidated TCP read");
    wait_for_write(first_write, *endpoints.endpoint, "The first invalidated TCP write");
    wait_for_write(second_write, *endpoints.endpoint, "The second invalidated TCP write");

    EXPECT_EQ(get_read_result(first_read).state, pipe_op_res::canceled);
    EXPECT_EQ(get_read_result(second_read).state, pipe_op_res::canceled);
    EXPECT_EQ(get_write_result(first_write), pipe_op_res::canceled);
    EXPECT_EQ(get_write_result(second_write), pipe_op_res::canceled);
    EXPECT_FALSE(endpoints.endpoint->is_connected());
    EXPECT_TRUE(callback_event->wait_for(operation_timeout));
    EXPECT_EQ(callback_count->load(std::memory_order_relaxed), 1u);

    endpoints.endpoint->invalidate();
    EXPECT_EQ(callback_count->load(std::memory_order_relaxed), 1u);

    auto failed_read = endpoints.endpoint->read_async();
    auto failed_write = endpoints.endpoint->write_async(make_message(1));
    auto failed_timed_read = endpoints.endpoint->read_async(long_timeout);
    auto failed_timed_write = endpoints.endpoint->write_async(make_message(1), long_timeout);
    EXPECT_EQ(get_read_result(failed_read).state, pipe_op_res::failed);
    EXPECT_EQ(get_write_result(failed_write), pipe_op_res::failed);
    EXPECT_EQ(get_read_result(failed_timed_read).state, pipe_op_res::failed);
    EXPECT_EQ(get_write_result(failed_timed_write), pipe_op_res::failed);
}

TEST_F(TcpPipeEndpoint, InvalidateWhileReadingPayloadCancelsOperation)
{
    auto endpoints = create_raw_endpoint();
    auto message = make_message(503, 17);
    const auto header = boost::endian::native_to_big(static_cast<uint32_t>(message.size()));
    auto read = endpoints.endpoint->read_async(long_timeout);

    ASSERT_TRUE(write_exact(endpoints.peer, &header, sizeof(header)));
    ASSERT_TRUE(write_exact(endpoints.peer, message.data(), 11));
    std::this_thread::sleep_for(partial_transfer_delay);
    endpoints.endpoint->invalidate();
    wait_for_read(read, *endpoints.endpoint, "The payload read canceled by invalidate");

    auto result = get_read_result(read);
    EXPECT_EQ(result.state, pipe_op_res::canceled);
    EXPECT_EQ(result.data.size(), 0u);
    EXPECT_FALSE(endpoints.endpoint->is_connected());
}

TEST_F(TcpPipeEndpoint, InvalidateWhileWritingPayloadAllowsRacedSuccessOrCancellation)
{
    auto endpoints = create_constrained_raw_endpoint();
    auto write = endpoints.endpoint->write_async(make_message(MaxTransferMessageSize, 18),
                                                  long_timeout);
    uint32_t header = 0;
    ASSERT_TRUE(read_exact(endpoints.peer, &header, sizeof(header)));
    ASSERT_EQ(boost::endian::big_to_native(header), MaxTransferMessageSize);

    endpoints.endpoint->invalidate();
    wait_for_write(write, *endpoints.endpoint, "The payload write completing during invalidate");

    const auto result = get_write_result(write);
    EXPECT_TRUE(result == pipe_op_res::success || result == pipe_op_res::canceled);
    EXPECT_FALSE(endpoints.endpoint->is_connected());
}

TEST_F(TcpPipeEndpoint, DestructorCancelsPendingTimedRead)
{
    auto endpoints = create_raw_endpoint();
    auto read = endpoints.endpoint->read_async(long_timeout);

    endpoints.endpoint.reset();

    wait_for_result(read, [] {}, "The timed TCP read canceled by endpoint destruction");
    auto result = get_read_result(read);
    EXPECT_EQ(result.state, pipe_op_res::canceled);
    EXPECT_EQ(result.data.size(), 0u);
}

TEST_F(TcpPipeEndpoint, GracefulPeerDisconnectFailsPendingReadsAndInvokesCallback)
{
    auto endpoints = create_raw_endpoint();
    auto callback_event = std::make_shared<event>();
    endpoints.endpoint->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(), [callback_event] { callback_event->set(); }));
    auto first_read = endpoints.endpoint->read_async();
    auto second_read = endpoints.endpoint->read_async(long_timeout);

    boost::system::error_code ignored;
    endpoints.peer.shutdown(tcp::socket::shutdown_both, ignored);
    endpoints.peer.close(ignored);

    wait_for_read(first_read, *endpoints.endpoint, "The first read after graceful disconnect");
    wait_for_read(second_read, *endpoints.endpoint, "The second read after graceful disconnect");
    EXPECT_EQ(get_read_result(first_read).state, pipe_op_res::failed);
    EXPECT_EQ(get_read_result(second_read).state, pipe_op_res::failed);
    EXPECT_FALSE(endpoints.endpoint->is_connected());
    EXPECT_TRUE(callback_event->wait_for(operation_timeout));
}

TEST_F(TcpPipeEndpoint, PeerDisconnectDuringPayloadFailsRead)
{
    auto endpoints = create_raw_endpoint();
    auto callback_event = std::make_shared<event>();
    endpoints.endpoint->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(), [callback_event] { callback_event->set(); }));
    auto message = make_message(601, 19);
    const auto header = boost::endian::native_to_big(static_cast<uint32_t>(message.size()));
    auto read = endpoints.endpoint->read_async(long_timeout);

    ASSERT_TRUE(write_exact(endpoints.peer, &header, sizeof(header)));
    ASSERT_TRUE(write_exact(endpoints.peer, message.data(), 13));
    std::this_thread::sleep_for(partial_transfer_delay);
    boost::system::error_code ignored;
    endpoints.peer.shutdown(tcp::socket::shutdown_both, ignored);
    endpoints.peer.close(ignored);

    wait_for_read(read, *endpoints.endpoint, "The payload read after peer disconnect");
    auto result = get_read_result(read);
    EXPECT_EQ(result.state, pipe_op_res::failed);
    EXPECT_EQ(result.data.size(), 0u);
    EXPECT_FALSE(endpoints.endpoint->is_connected());
    EXPECT_TRUE(callback_event->wait_for(operation_timeout));
}

TEST_F(TcpPipeEndpoint, PeerResetFailsPendingWritesAndInvokesCallback)
{
    auto endpoints = create_blocked_raw_endpoint();
    ASSERT_NE(endpoints.filler_size, 0u);
    auto callback_event = std::make_shared<event>();
    endpoints.endpoint->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(), [callback_event] { callback_event->set(); }));
    auto first_write = endpoints.endpoint->write_async(make_message(MaxTransferMessageSize, 11));
    auto second_write = endpoints.endpoint->write_async(make_message(79, 12), long_timeout);

    endpoints.peer.set_option(boost::asio::socket_base::linger(true, 0));
    boost::system::error_code ignored;
    endpoints.peer.close(ignored);

    wait_for_write(first_write, *endpoints.endpoint, "The first write after peer reset");
    wait_for_write(second_write, *endpoints.endpoint, "The second write after peer reset");
    EXPECT_EQ(get_write_result(first_write), pipe_op_res::failed);
    EXPECT_EQ(get_write_result(second_write), pipe_op_res::failed);
    EXPECT_FALSE(endpoints.endpoint->is_connected());
    EXPECT_TRUE(callback_event->wait_for(operation_timeout));
}

TEST_F(TcpPipeEndpoint, PeerResetDuringPayloadAllowsRacedFirstWriteSuccess)
{
    auto endpoints = create_constrained_raw_endpoint();
    auto first_write = endpoints.endpoint->write_async(make_message(MaxTransferMessageSize, 20));
    auto second_write = endpoints.endpoint->write_async(make_message(89, 21), long_timeout);
    uint32_t header = 0;
    ASSERT_TRUE(read_exact(endpoints.peer, &header, sizeof(header)));
    ASSERT_EQ(boost::endian::big_to_native(header), MaxTransferMessageSize);

    endpoints.peer.set_option(boost::asio::socket_base::linger(true, 0));
    boost::system::error_code ignored;
    endpoints.peer.close(ignored);

    wait_for_write(first_write, *endpoints.endpoint, "The payload write after peer reset");
    wait_for_write(second_write, *endpoints.endpoint, "The queued write after payload reset");
    const auto first_result = get_write_result(first_write);
    EXPECT_TRUE(first_result == pipe_op_res::success || first_result == pipe_op_res::failed);
    EXPECT_EQ(get_write_result(second_write), pipe_op_res::failed);
    EXPECT_FALSE(endpoints.endpoint->is_connected());
}

TEST_F(TcpPipeEndpoint, DisconnectAfterReadTimeoutKeepsTimeoutResultAndInvalidatesEndpoint)
{
    auto endpoints = create_raw_endpoint();
    auto callback_event = std::make_shared<event>();
    endpoints.endpoint->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(), [callback_event] { callback_event->set(); }));
    auto read = endpoints.endpoint->read_async(immediate_timeout);

    wait_for_read(read, *endpoints.endpoint, "The read timing out before peer disconnect");
    EXPECT_EQ(get_read_result(read).state, pipe_op_res::timeout);

    boost::system::error_code ignored;
    endpoints.peer.close(ignored);
    EXPECT_TRUE(callback_event->wait_for(operation_timeout));
    EXPECT_FALSE(endpoints.endpoint->is_connected());
}

TEST_F(TcpPipeEndpoint, DisconnectCallbackSetAfterInvalidationExecutesImmediately)
{
    auto endpoints = create_raw_endpoint();
    auto callback_event = std::make_shared<event>();

    endpoints.endpoint->invalidate();
    endpoints.endpoint->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(), [callback_event] { callback_event->set(); }));

    EXPECT_TRUE(callback_event->wait_for(operation_timeout));
}

TEST_F(TcpPipeEndpoint, InvokesEveryDisconnectCallbackOnlyOnce)
{
    auto endpoints = create_raw_endpoint();
    auto first_event = std::make_shared<event>();
    auto second_event = std::make_shared<event>();
    auto callback_count = std::make_shared<std::atomic<unsigned>>(0);
    endpoints.endpoint->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(),
        [first_event, callback_count] {
            callback_count->fetch_add(1, std::memory_order_relaxed);
            first_event->set();
        }));
    endpoints.endpoint->set_disconnect_callback(thread_pool_task(
        m_thread_pool.get(),
        [second_event, callback_count] {
            callback_count->fetch_add(1, std::memory_order_relaxed);
            second_event->set();
        }));

    endpoints.endpoint->invalidate();
    EXPECT_TRUE(first_event->wait_for(operation_timeout));
    EXPECT_TRUE(second_event->wait_for(operation_timeout));
    EXPECT_EQ(callback_count->load(std::memory_order_relaxed), 2u);

    endpoints.endpoint->invalidate();
    EXPECT_EQ(callback_count->load(std::memory_order_relaxed), 2u);
}
