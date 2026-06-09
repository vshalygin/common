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
