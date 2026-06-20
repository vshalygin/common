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
}

TEST(MemPipeEnv, OpenNewPipeUnconnectedIfNoCounterpart)
{
    auto pool = std::make_shared<thread_pool>(2);
    mem_pipe_env sut(pool);

    auto pipe_enpoint = sut.open_pipe();

    ASSERT_FALSE(pipe_enpoint->is_connected());
    ASSERT_TRUE(sut.get_client_pipe_endpoint_queue_size() == 1);
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
}

TEST(MemPipeEnv, CreatedAndOpenedPipesAreConnected)
{
    auto pool = std::make_shared<thread_pool>(2);
    mem_pipe_env sut(pool);
    auto client_pipe = sut.open_pipe();
    auto server_pipe = sut.create_pipe();
    buffer client_msg(1); client_msg[0] = (std::byte)0x1;
    buffer server_msg(1); server_msg[0] = (std::byte)0x2;

    ASSERT_TRUE(client_pipe->try_to_write_for(client_msg.copy(), std::chrono::seconds(1)));
    ASSERT_TRUE(server_pipe->try_to_write_for(server_msg.copy(), std::chrono::seconds(1)));
    auto r1 = client_pipe->try_to_read_for(std::chrono::seconds(1));
    auto r2 = server_pipe->try_to_read_for(std::chrono::seconds(1));

    ASSERT_TRUE(r1 && *r1 == server_msg);
    ASSERT_TRUE(r2 && *r2 == client_msg);
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
}
