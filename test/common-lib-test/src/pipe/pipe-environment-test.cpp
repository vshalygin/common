//#include <common-lib/pipe/pipe-environment.h>
//#include <common-lib/thread-pool/thread-pool.h>
//#include <common-lib/syncronization/latch/latch.h>
//#include <common-lib/syncronization/event/event.h>
//
//#include <gtest/gtest.h>
//#include <gmock/gmock.h>
//
//#include <string>
//
//using namespace vsh::cl;
//using namespace testing;
//
//namespace {
//    const std::string s_pipe_name = "really unique pipe name";
//
//    //TODO move to test lib
//    MATCHER_P2(ArrayEq, expected, size, "Arrays are equal") {
//        for(size_t i = 0; i < size; ++i) {
//            if(arg[i] != expected[i]) {
//                return false;
//            }
//        }
//        return true;
//    }
//}
//
//TEST(PipeEnvironment, AnswersZeroExistingPipesCountAfterCreation)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = pipe_environment::create(pool);
//
//    ASSERT_EQ(sut->get_existing_pipes_count(), 0);
//}
//
//TEST(PipeEnvironment, AnswersOneExistingPipesCountIfPipeWasCreated)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = pipe_environment::create(pool);
//
//    auto server_pipe_endpoint = sut->create_pipe(s_pipe_name);
//
//    ASSERT_EQ(sut->get_existing_pipes_count(), 1);
//}
//
//TEST(PipeEnvironment, ThrowsExceptionOnAttemptToCreatePipeWithSameNameTwice)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = pipe_environment::create(pool);
//
//    auto server_pipe_endpoint = sut->create_pipe(s_pipe_name);
//    ASSERT_ANY_THROW(sut->create_pipe(s_pipe_name));
//}
//
//TEST(PipeEnvironment, AnswersZeroExistingPipesCountAfterServerEndpointDestroyed)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = pipe_environment::create(pool);
//
//    auto server_pipe_endpoint = sut->create_pipe(s_pipe_name);
//    server_pipe_endpoint.reset();
//
//    ASSERT_EQ(sut->get_existing_pipes_count(), 0);
//}
//
//TEST(PipeEnvironment, CreatesServerEndpointDisconnected)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = pipe_environment::create(pool);
//
//    auto server_pipe_endpoint = sut->create_pipe(s_pipe_name);
//    
//    ASSERT_FALSE(server_pipe_endpoint->is_connected());
//}
//
//TEST(PipeEnvironment, ThrowsExceptionOnAttemptToOpenUnexistingPipe)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = pipe_environment::create(pool);
//
//    ASSERT_ANY_THROW(sut->open_pipe(s_pipe_name));
//}
//
//TEST(PipeEnvironment, ConnectsServerEndpointAfterClientEnpointCreated)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = pipe_environment::create(pool);
//
//    auto server_endpoint = sut->create_pipe(s_pipe_name);
//    auto client_enpoint = sut->open_pipe(s_pipe_name);
//
//    ASSERT_TRUE(server_endpoint->is_connected());
//}
//
//TEST(PipeEnvironment, CreatesClientEndpointConnected)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = pipe_environment::create(pool);
//
//    auto server_endpoint = sut->create_pipe(s_pipe_name);
//    auto client_enpoint = sut->open_pipe(s_pipe_name);
//
//    ASSERT_TRUE(client_enpoint->is_connected());
//}
//
//TEST(PipeEnvironment, ThrowsExceptionOnAttemptToOpenPipeWithSameNameTwice)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = pipe_environment::create(pool);
//    auto server_endpoint = sut->create_pipe(s_pipe_name);
//    auto client_enpoint = sut->open_pipe(s_pipe_name);
//
//    ASSERT_ANY_THROW(sut->open_pipe(s_pipe_name));
//}
//
//TEST(PipeEnvironment, DisablesServerEndpointAfterClientEndpointDestroyed)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = pipe_environment::create(pool);
//    auto server_endpoint = sut->create_pipe(s_pipe_name);
//    auto client_enpoint = sut->open_pipe(s_pipe_name);
//
//    client_enpoint.reset();
//
//    ASSERT_FALSE(server_endpoint->is_connected());
//}
//
//TEST(PipeEnvironment, DisablesClientEndpointAfterServerEndpointDestroyed)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = pipe_environment::create(pool);
//    auto server_endpoint = sut->create_pipe(s_pipe_name);
//    auto client_enpoint = sut->open_pipe(s_pipe_name);
//
//    server_endpoint.reset();
//
//    ASSERT_FALSE(client_enpoint->is_connected());
//}
//
//TEST(PipeEnvironment, OpensPipeSuccessfullyAfterPreviousEndpointDestroyed)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = pipe_environment::create(pool);
//    auto server_endpoint = sut->create_pipe(s_pipe_name);
//    auto client_enpoint = sut->open_pipe(s_pipe_name);
//
//    client_enpoint.reset();
//    client_enpoint = sut->open_pipe(s_pipe_name);
//
//    ASSERT_TRUE(client_enpoint->is_connected());
//    ASSERT_TRUE(server_endpoint->is_connected());
//}
//
//TEST(PipeEnvironment, CreatesEndpointWhichExchangeInformationWithEachOther)
//{
//    latch sync_latch(2);
//    buffer msg_to_server(1); msg_to_server[0] = std::byte(0x16);
//    buffer msg_to_client(1); msg_to_client[0] = std::byte(0x17);
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = pipe_environment::create(pool);
//    auto server_endpoint = sut->create_pipe(s_pipe_name);
//    auto client_enpoint = sut->open_pipe(s_pipe_name);
//
//    MockFunction<void(pipe_result, buffer &&)> server_read_callback;
//    EXPECT_CALL(server_read_callback,
//                Call(pipe_result::ok, ArrayEq(msg_to_server.data(), msg_to_server.size())))
//        .Times(1)
//        .WillOnce([&]() { sync_latch.count_down(); });
//    MockFunction<void(pipe_result, buffer &&)> client_read_callback;
//    EXPECT_CALL(client_read_callback,
//                Call(pipe_result::ok, ArrayEq(msg_to_client.data(), msg_to_client.size())))
//        .Times(1)
//        .WillOnce([&]() { sync_latch.count_down(); });;
//
//    server_endpoint->write_async(msg_to_client.copy(), {});
//    client_enpoint->write_async(msg_to_server.copy(), {});
//    server_endpoint->read_async(server_read_callback.AsStdFunction());
//    client_enpoint->read_async(client_read_callback.AsStdFunction());
//
//    ASSERT_TRUE(sync_latch.wait_for(std::chrono::seconds(10)));
//}
