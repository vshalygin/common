#include <rpc-lib/pipe/memory-pipe/mem-pipe-env.h>
#include <rpc-lib/pipe/ipipe-endpoint.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::rpc;
using namespace vshalygin::cl;
using namespace testing;

TEST(MemPipeEnv, CreatesNewPipeUnconnectedIfNoCounterpart)
{
    auto pool = std::make_shared<thread_pool>(2);
    mem_pipe_env sut(pool);

    auto pipe_enpoint = sut.create_pipe();

    ASSERT_FALSE(pipe_enpoint->is_connected());
    ASSERT_TRUE(sut.get_server_pipe_endpoint_queue_size() == 1);
    pool->stop();
}

TEST(MemPipeEnv, OpenNewPipeUnconnectedIfNoCounterpart)
{
    auto pool = std::make_shared<thread_pool>(2);
    mem_pipe_env sut(pool);

    auto pipe_enpoint = sut.open_pipe();

    ASSERT_FALSE(pipe_enpoint->is_connected());
    ASSERT_TRUE(sut.get_client_pipe_endpoint_queue_size() == 1);
    pool->stop();
}

TEST(MemPipeEnv, CreateNewPipeConnectedIfCounterpartExists)
{
    auto pool = std::make_shared<thread_pool>(2);
    mem_pipe_env sut(pool);
    auto client_pipe_enpoint = sut.open_pipe();

    auto server_pipe_enpoint = sut.create_pipe();

    EXPECT_TRUE(client_pipe_enpoint->is_connected());
    EXPECT_TRUE(server_pipe_enpoint->is_connected());
    EXPECT_TRUE(sut.get_client_pipe_endpoint_queue_size() == 0);
    EXPECT_TRUE(sut.get_server_pipe_endpoint_queue_size() == 0);
    pool->stop();
}

TEST(MemPipeEnv, OpenNewPipeConnectedIfCounterpartExists)
{
    auto pool = std::make_shared<thread_pool>(2);
    mem_pipe_env sut(pool);
    auto client_pipe = sut.open_pipe();

    auto server_pipe = sut.create_pipe();

    EXPECT_TRUE(client_pipe->is_connected());
    EXPECT_TRUE(server_pipe->is_connected());
    EXPECT_TRUE(sut.get_client_pipe_endpoint_queue_size() == 0);
    EXPECT_TRUE(sut.get_server_pipe_endpoint_queue_size() == 0);
    pool->stop();
}

TEST(MemPipeEnv, CreatedAndOpenedPipesAreConnected)
{
    auto pool = std::make_shared<thread_pool>(2);
    mem_pipe_env sut(pool);
    auto client_pipe = sut.open_pipe();
    auto server_pipe = sut.create_pipe();
    buffer client_msg(1); client_msg[0] = (std::byte)0x1;
    buffer server_msg(1); server_msg[0] = (std::byte)0x2;

    client_pipe->write_async(client_msg.copy());
    server_pipe->write_async(server_msg.copy());
    client_pipe->read_async().get().apply([&](pipe_op_res res, buffer &b) {
        EXPECT_TRUE(is_success(res));
        EXPECT_TRUE(b == server_msg);
    });
    server_pipe->read_async().get().apply([&](pipe_op_res res, buffer &b) {
        EXPECT_TRUE(is_success(res));
        EXPECT_TRUE(b == client_msg);
    });
    pool->stop();
}

TEST(MemPipeEnv, DoNotConnectNewPipeToCounterpartIfItIsNotExistingAnymore)
{
    auto pool = std::make_shared<thread_pool>(2);
    mem_pipe_env sut(pool);
    auto client_pipe = sut.open_pipe();
    client_pipe.reset();

    auto server_pipe = sut.create_pipe();

    EXPECT_FALSE(server_pipe->is_connected());
    EXPECT_TRUE(sut.get_client_pipe_endpoint_queue_size() == 0);
    EXPECT_TRUE(sut.get_server_pipe_endpoint_queue_size() == 1);
    pool->stop();
}
