#include <rpc-lib/client-endpoint/client-endpoint.h>
#include <rpc-lib/transfer-message/transfer-message.h>
#include <rpc-lib/closure-guard/closure-guard.h>
#include <rpc-lib/types/request-exception.h>

#include "mocks/channel-mock.h"
#include "mocks/connection-mock.h"
#include "mocks/connector-mock.h"
#include "mocks/service-mock.h"
#include "mocks/transport-mock.h"

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#include <google/protobuf/util/message_differencer.h>
#pragma warning(pop)

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::rpc;
using namespace vshalygin::cl;
using namespace google::protobuf::util;
using namespace testing;

class ClientEndpoint
    : public Test
{
protected:
    void SetUp() override
    {
        m_service = std::make_shared<service_nice_mock>();
        m_channel = std::make_unique<channel_nice_mock>();
        m_connection = std::make_unique<connection_nice_mock>();
        m_connector = std::make_unique<connector_nice_mock>();

        m_request_message.set_data(34);
        m_response_message.set_data(45);

        ON_CALL(m_response_handler, Call)
            .WillByDefault([&](auto &&buf) { EXPECT_EQ(buf, m_raw_response_message); });
        ON_CALL(*m_service, process_request)
            .WillByDefault([&](auto &&buf, auto &&res_handler) {
                               EXPECT_EQ(buf, m_raw_request_message);
                               res_handler(m_raw_response_message.copy());
                           });
        ON_CALL(*m_connection, set_request_handler)
            .WillByDefault([&](auto &&handler) {
                               m_request_handler = std::move(handler);
                           });
        ON_CALL(*m_channel, CallMethod)
            .WillByDefault([&](const google::protobuf::MethodDescriptor *method,
                               google::protobuf::RpcController * controller,
                               const google::protobuf::Message *request,
                               google::protobuf::Message *response,
                               google::protobuf::Closure *done) {
                                  closure_guard guard(done);
                                  EXPECT_EQ(proto::Service_Stub::descriptor()->method(m_stub_method), method);
                                  EXPECT_TRUE(MessageDifferencer::Equals(*request, m_request_message));
                                  if(is_fail(m_response_result)) {
                                      controller->SetFailed(to_string(request_result::request_not_processed));
                                  } else {
                                      response->CopyFrom(m_response_message);
                                  }
                           });
    }

    std::unique_ptr<client_endpoint> create_sut()
    {
        return std::make_unique<client_endpoint>(m_service,
                                                 std::move(m_channel),
                                                 m_connection,
                                                 std::move(m_connector));
    }

protected:
    const int m_stub_method = 1;
    const decltype(&proto::Service_Stub::Method2) m_stub_method_ptr = &proto::Service_Stub::Method2;
    response_result m_response_result = response_result::ok;

    std::function<void(buffer &&, std::function<void(buffer &&)> &&)> m_request_handler;

    proto::request_message m_request_message;
    proto::response_message m_response_message;
    buffer m_raw_request_message;
    buffer m_raw_response_message;
    MockFunction<void(buffer &&)> m_response_handler;

    std::shared_ptr<service_nice_mock> m_service;
    std::unique_ptr<channel_nice_mock> m_channel;
    std::shared_ptr<connection_nice_mock> m_connection;
    std::unique_ptr<connector_nice_mock> m_connector;
};

TEST_F(ClientEndpoint, SetsRequestHandlerAfterConstruction)
{
    MockFunction<void(buffer &&)> response_handler;
    EXPECT_CALL(response_handler, Call)
        .Times(1);
    EXPECT_CALL(*m_service, process_request)
        .Times(1);
    EXPECT_CALL(*m_connection, set_request_handler)
        .Times(1)
        .WillOnce([&](auto &&handler) {
                      handler(m_raw_request_message.copy(), response_handler.AsStdFunction());
                  });

    auto sut = create_sut();
}

TEST_F(ClientEndpoint, StartTransportAndSetsConnection)
{
    auto transport = std::make_unique<transport_mock>();
    auto transport_ptr = transport.get();
    EXPECT_CALL(*m_connector, create_transport())
        .Times(1)
        .WillOnce(Return(ByMove(std::move(transport))));
    EXPECT_CALL(*m_connection, start_and_set_transport)
        .Times(1)
        .WillOnce([&](auto &&transp) { EXPECT_EQ(transp.get(), transport_ptr);  });
    EXPECT_CALL(*m_channel, set_connection)
        .Times(1)
        .WillOnce([&](auto &&conn) { EXPECT_EQ(m_connection.get(), conn.get());  });

    auto sut = create_sut();
    sut->connect();
}

TEST_F(ClientEndpoint, DisconnectsConnection)
{
    EXPECT_CALL(*m_connection, stop_transport)
        .Times(1);

    auto sut = create_sut();
    sut->disconnect();
}

TEST_F(ClientEndpoint, ChecksConnection)
{
    EXPECT_CALL(*m_connection, is_active)
        .Times(1)
        .WillOnce(Return(true));

    auto sut = create_sut();
    const auto ans = sut->is_connected();

    ASSERT_TRUE(ans);
}

TEST_F(ClientEndpoint, SetsConnectionChangeStateHandler)
{
    MockFunction<void(connection_state)> handler;
    EXPECT_CALL(handler, Call(connection_state::disconnected))
        .Times(1);
    EXPECT_CALL(*m_connection, set_change_state_handler)
        .Times(1)
        .WillOnce([](auto &&h) { h(connection_state::disconnected); });

    auto sut = create_sut();
    sut->set_connection_change_state_handler(handler.AsStdFunction());
}

TEST_F(ClientEndpoint, MakeSuccessRequest)
{
    proto::Service_Stub stub(m_channel.get());
    m_raw_request_message = create_transfer_msg_req(34, 1, &m_request_message);
    m_raw_response_message = create_transfer_msg_res(34, m_response_result, &m_response_message);

    auto sut = create_sut();
    auto ans = sut->make_request<proto::request_message, proto::response_message>
        (m_request_message, stub, m_stub_method_ptr);

    ASSERT_TRUE(ans);
    ASSERT_TRUE(MessageDifferencer::Equals(m_response_message, *ans));
}

TEST_F(ClientEndpoint, ThrowsRequestExcepionOnFailedRequest)
{
    m_response_result = response_result::insufficient_rights;

    proto::Service_Stub stub(m_channel.get());
    m_raw_request_message = create_transfer_msg_req(34, m_stub_method, &m_request_message);
    m_raw_response_message = create_transfer_msg_res(34, m_response_result, &m_response_message);

    auto sut = create_sut();
    try {
        sut->make_request<proto::request_message, proto::response_message>
            (m_request_message, stub, m_stub_method_ptr);
        FAIL();
    } catch (const request_exception &e) {
        ASSERT_TRUE(e.code() == request_result::request_not_processed);
    } catch (...) {
        FAIL();
    }
}

TEST_F(ClientEndpoint, MakeSuccessRequestAsync)
{
    event sync_event;
    m_response_result = response_result::ok;

    proto::Service_Stub stub(m_channel.get());
    m_raw_request_message = create_transfer_msg_req(34, m_stub_method, &m_request_message);
    m_raw_response_message = create_transfer_msg_res(34, m_response_result, &m_response_message);
    auto callback = [&](request_result rc, std::unique_ptr<proto::response_message> response) {
        EXPECT_TRUE(response);
        EXPECT_TRUE(MessageDifferencer::Equals(m_response_message, *response));
        EXPECT_EQ(rc, request_result::ok);
        sync_event.set();
    };
    auto sut = create_sut();
    sut->make_request_async<proto::request_message, proto::response_message>
        (m_request_message, stub, m_stub_method_ptr, std::move(callback));

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(ClientEndpoint, MakeFailRequestAsync)
{
    event sync_event;
    m_response_result = response_result::insufficient_rights;

    proto::Service_Stub stub(m_channel.get());
    m_raw_request_message = create_transfer_msg_req(34, m_stub_method, &m_request_message);
    m_raw_response_message = create_transfer_msg_res(34, m_response_result, &m_response_message);
    auto callback = [&](request_result rc, std::unique_ptr<proto::response_message>) {
        EXPECT_EQ(rc, request_result::request_not_processed);
        sync_event.set();
    };
    auto sut = create_sut();
    sut->make_request_async<proto::request_message, proto::response_message>
        (m_request_message, stub, m_stub_method_ptr, std::move(callback));
    
    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(ClientEndpoint, RequestHandlerCatchesException)
{
    EXPECT_CALL(*m_service, process_request)
        .Times(1)
        .WillOnce(Throw(std::exception()));

    auto sut = create_sut(); sut;

    ASSERT_NO_THROW(m_request_handler({}, {}));
}
