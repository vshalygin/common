#include <common-lib/pipe/pipe-env.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::cl;
using namespace testing;

TEST(PipeEnv, CreatesNewPipeUnconnectedIfNoCounterpart)
{
    auto pool = std::make_shared<thread_pool>(2);
    pipe_env sut(pool);

    auto pipe = sut.create_pipe("name");

    ASSERT_FALSE(pipe->is_connected());
    ASSERT_TRUE(sut.get_server_pipe_map_size() == 1);
    ASSERT_TRUE(sut.get_server_pipe_queue_size("name") == 1);
}

TEST(PipeEnv, OpenNewPipeUnconnectedIfNoCounterpart)
{
    auto pool = std::make_shared<thread_pool>(2);
    pipe_env sut(pool);

    auto pipe = sut.open_pipe("name");

    ASSERT_FALSE(pipe->is_connected());
    ASSERT_TRUE(sut.get_client_pipe_map_size() == 1);
    ASSERT_TRUE(sut.get_client_pipe_queue_size("name") == 1);
}

TEST(PipeEnv, CreateNewPipeConnectedIfCounterpartExists)
{
    auto pool = std::make_shared<thread_pool>(2);
    pipe_env sut(pool);
    auto client_pipe = sut.open_pipe("name");

    auto server_pipe = sut.create_pipe("name");

    EXPECT_TRUE(client_pipe->is_connected());
    EXPECT_TRUE(server_pipe->is_connected());
    EXPECT_TRUE(sut.get_client_pipe_map_size() == 0);
    EXPECT_TRUE(sut.get_client_pipe_queue_size("name") == 0);
    EXPECT_TRUE(sut.get_server_pipe_map_size() == 0);
    EXPECT_TRUE(sut.get_server_pipe_queue_size("name") == 0);
}

TEST(PipeEnv, OpenNewPipeConnectedIfCounterpartExists)
{
    auto pool = std::make_shared<thread_pool>(2);
    pipe_env sut(pool);
    auto client_pipe = sut.open_pipe("name");

    auto server_pipe = sut.create_pipe("name");

    EXPECT_TRUE(client_pipe->is_connected());
    EXPECT_TRUE(server_pipe->is_connected());
    EXPECT_TRUE(sut.get_client_pipe_map_size() == 0);
    EXPECT_TRUE(sut.get_client_pipe_queue_size("name") == 0);
    EXPECT_TRUE(sut.get_server_pipe_map_size() == 0);
    EXPECT_TRUE(sut.get_server_pipe_queue_size("name") == 0);
}

TEST(PipeEnv, CreatedAndOpenedPipesAreConnected)
{
    auto pool = std::make_shared<thread_pool>(2);
    pipe_env sut(pool);
    auto client_pipe = sut.open_pipe("name");
    auto server_pipe = sut.create_pipe("name");
    buffer client_msg(1); client_msg[0] = (std::byte)0x1;
    buffer server_msg(1); server_msg[0] = (std::byte)0x2;

    ASSERT_TRUE(client_pipe->try_to_write_for(client_msg.copy(), std::chrono::seconds(1)));
    ASSERT_TRUE(server_pipe->try_to_write_for(server_msg.copy(), std::chrono::seconds(1)));
    auto r1 = client_pipe->try_to_read_for(std::chrono::seconds(1));
    auto r2 = server_pipe->try_to_read_for(std::chrono::seconds(1));

    ASSERT_TRUE(r1 && *r1 == server_msg);
    ASSERT_TRUE(r2 && *r2 == client_msg);
}

TEST(PipeEnv, CreateSeparatePipesIfTheirNamesDiffer)
{
    auto pool = std::make_shared<thread_pool>(2);
    pipe_env sut(pool);
    auto client_pipe = sut.open_pipe("name1");

    auto server_pipe = sut.create_pipe("name2");

    EXPECT_FALSE(client_pipe->is_connected());
    EXPECT_FALSE(server_pipe->is_connected());
    EXPECT_TRUE(sut.get_client_pipe_map_size() == 1);
    EXPECT_TRUE(sut.get_client_pipe_queue_size("name1") == 1);
    EXPECT_TRUE(sut.get_server_pipe_map_size() == 1);
    EXPECT_TRUE(sut.get_server_pipe_queue_size("name2") == 1);
}
