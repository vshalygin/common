//#include <rpc-lib/channel/channel.h>
//#include <rpc-lib/transfer-message/transfer-message.h>
//#include <rpc-lib/transport/itransport.h>
//
//#include "mocks/closure-mock.h"
//#include "mocks/connection-mock.h"
//#include "mocks/rpc-controller-mock.h"
//
//#pragma warning(push, 0)
//#include "proto/test-messages.pb.h"
//#include <google/protobuf/util/message_differencer.h>
//#pragma warning(pop)
//
//#include <gtest/gtest.h>
//
//using namespace vshalygin::rpc;
//using namespace vshalygin::cl;
//using namespace testing;
//using namespace google::protobuf::util;
//
//namespace {
//    class Service
//        : public proto::Service
//    {};
//}
//
//class Channel
//    : public Test
//{
//protected:
//    void SetUp() override
//    {
//        m_closure = std::make_unique<closure_nice_mock>();
//        m_rpc_controller = std::make_unique<rpc_controller_nice_mock>();
//
//        m_connection = std::make_shared<connection_nice_mock>();
//
//        ON_CALL(*m_connection, request_async)
//            .WillByDefault(SaveArg<1>(&m_callback));
//
//        m_some_message.set_string_data2("sdfasdfasdfsadfasdfsad");
//    }
//
//    void set_connection_and_call_method()
//    {
//        m_sut.set_connection(m_connection);
//        m_sut.CallMethod(m_service.descriptor()->method(0),
//                         m_rpc_controller.get(),
//                         &m_request_message,
//                         &m_response_message,
//                         m_closure.get());
//    }
//
//protected:
//    Service m_service;
//    std::unique_ptr<closure_nice_mock> m_closure;
//    std::unique_ptr<rpc_controller_nice_mock> m_rpc_controller;
//
//    std::shared_ptr<connection_nice_mock> m_connection;
//
//    proto::request_message m_request_message;
//    proto::response_message m_response_message;
//    proto::some_message m_some_message;
//
//    channel m_sut;
//
//    std::function<void(request_result, buffer &&)> m_callback;
//};
//
//TEST_F(Channel, ThrowsExceptionOnAttemptToCallMethodIfConnectionIsNotSet)
//{
//    ASSERT_ANY_THROW(m_sut.CallMethod(m_service.descriptor()->method(0),
//                                      m_rpc_controller.get(),
//                                      &m_request_message,
//                                      &m_response_message,
//                                      m_closure.get()));
//}
//
//TEST_F(Channel, AnswerNullptrConnectionIfItIsNotSet)
//{
//    ASSERT_THAT(m_sut.get_connection(), IsNull());
//}
//
//TEST_F(Channel, AnswerSetConnection)
//{
//    m_sut.set_connection(m_connection);
//    ASSERT_EQ(m_sut.get_connection().get(), m_connection.get());
//}
//
//TEST_F(Channel, DoesNotThrowExceptionOnAttemptToCallMethodIfConnectionIsSet)
//{
//    m_sut.set_connection(m_connection);
//    ASSERT_NO_THROW(m_sut.CallMethod(m_service.descriptor()->method(0),
//                                     m_rpc_controller.get(),
//                                     &m_request_message,
//                                     &m_response_message,
//                                     m_closure.get()));
//}
//
//TEST_F(Channel, ThrowExceptionsOnAttemptToCallMethodIfConnectionWasDropped)
//{
//    m_sut.set_connection(m_connection);
//    m_sut.drop_connection();
//    ASSERT_ANY_THROW(m_sut.CallMethod(m_service.descriptor()->method(0),
//                                      m_rpc_controller.get(),
//                                      &m_request_message,
//                                      &m_response_message,
//                                      m_closure.get()));
//}
//
//TEST_F(Channel, CallsClosureDoneAfterCallbackDestroyed)
//{
//    set_connection_and_call_method();
//
//    m_callback = std::function<void(request_result, buffer &&)>();
//
//    Mock::VerifyAndClearExpectations(m_closure.get());
//}
//
//TEST_F(Channel, MakesRequestWithCorrectRequestMessage)
//{
//    EXPECT_CALL(*m_connection, request_async)
//        .Times(1)
//        .WillOnce([&](auto &&buf, auto &&callback) {
//                      EXPECT_EQ(create_transfer_msg_req(0, 0, &m_request_message), buf);
//                      m_callback = std::move(callback);
//                  });
//
//    set_connection_and_call_method();
//}
//
//TEST_F(Channel, IncrementsMessageNumberOnEveryRequest)
//{
//    EXPECT_CALL(*m_connection, request_async)
//        .Times(2)
//        .WillOnce([&](auto &&buf, auto &&) { EXPECT_EQ(get_msg_number_req(buf), 0); })
//        .WillOnce([&](auto &&buf, auto &&) { EXPECT_EQ(get_msg_number_req(buf), 1); });
//
//    set_connection_and_call_method();
//    set_connection_and_call_method();
//}
//
//TEST_F(Channel, SetsControllerFailedIfCallbackCalledWithErrorCode)
//{
//    EXPECT_CALL(*m_rpc_controller, SetFailed(to_string(request_result::timeout)))
//        .Times(1);
//
//    set_connection_and_call_method();
//
//    m_callback(request_result::timeout, {});
//}
//
//TEST_F(Channel, SetsControllerFailedIfParsingResponseFailed)
//{
//    EXPECT_CALL(*m_rpc_controller, SetFailed(to_string(request_result::response_parse_error)))
//        .Times(1);
//
//    set_connection_and_call_method();
//
//    m_callback(request_result::ok, create_transfer_msg_res(0, response_result::ok, &m_some_message));
//}
//
//TEST_F(Channel, SetsControllerFailedWithRequestNotProcessedCodeIfResponseHasFailedCode)
//{
//    EXPECT_CALL(*m_rpc_controller, SetFailed(to_string(request_result::request_not_processed)))
//        .Times(1);
//
//    set_connection_and_call_method();
//
//    proto::response_message response_message;
//    response_message.set_data(34);
//
//    m_callback(request_result::ok,
//               create_transfer_msg_res(0, response_result::unknown_error, &response_message));
//
//    ASSERT_FALSE(MessageDifferencer::Equals(m_response_message, response_message));
//}
//
//TEST_F(Channel, SetsControllerFailedWithUnknownErrorCodeIfExceptionHappen)
//{
//    EXPECT_CALL(*m_rpc_controller, SetFailed(to_string(request_result::request_not_processed)))
//        .Times(1)
//        .WillOnce(Throw(std::exception()));
//    EXPECT_CALL(*m_rpc_controller, SetFailed(to_string(request_result::unknown_error)))
//        .Times(1);
//
//    set_connection_and_call_method();
//
//    proto::response_message response_message;
//    m_callback(request_result::ok,
//               create_transfer_msg_res(0, response_result::unknown_error, &response_message));
//}
//
//TEST_F(Channel, ParsesResponse)
//{
//    EXPECT_CALL(*m_rpc_controller, SetFailed)
//        .Times(0);
//
//    set_connection_and_call_method();
//
//    proto::response_message expected_response_message;
//    expected_response_message.set_data(23);
//
//    m_callback(request_result::ok,
//               create_transfer_msg_res(0, response_result::ok, &expected_response_message));
//
//    ASSERT_TRUE(MessageDifferencer::Equals(m_response_message, expected_response_message));
//}
