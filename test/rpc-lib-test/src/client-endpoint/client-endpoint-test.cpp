#include <rpc-lib/client-endpoint/client-endpoint.h>
#include <rpc-lib/transfer-message/transfer-message.h>
#include <rpc-lib/closure-guard/closure-guard.h>
#include <rpc-lib/types/request-exception.h>

#include "mocks/channel-mock.h"
#include "mocks/connector-mock.h"
#include "mocks/service-mock.h"
#include "mocks/transport-mock.h"
#include "mocks/connection-mock.h"

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
        m_transport = std::make_unique<transport_nice_mock>();
        m_transport_ptr = m_transport.get();
        ON_CALL(*m_transport, recv_async)
            .WillByDefault(SaveArg<0>(&m_recv_handler));
        ON_CALL(*m_transport, start)
            .WillByDefault([this](auto &&start_handler, auto &&stop_handler) {
                               m_stop_transport_handler = std::move(stop_handler);
                               start_handler();
                           });
        ON_CALL(*m_transport, stop)
            .WillByDefault([this]() {
                               ASSERT_TRUE(m_stop_transport_handler);
                               m_stop_transport_handler();
                           });

        m_thread_pool = std::make_shared<thread_pool>(1);

        m_service = std::make_shared<service_nice_mock>();
        m_channel = std::make_unique<channel_nice_mock>();
        m_connector = std::make_unique<connector_nice_mock>();
        ON_CALL(*m_connector, create_transport)
            .WillByDefault([this]() { return std::move(m_transport); });


        m_request_message.set_data(34);
        m_response_message.set_data(45);

        ON_CALL(m_response_handler, Call)
            .WillByDefault([&](auto &&buf) { EXPECT_EQ(buf, m_raw_response_message); });
        ON_CALL(*m_service, process_request)
            .WillByDefault([&](auto &&buf, auto &&res_handler) {
                               EXPECT_EQ(buf, m_raw_request_message);
                               res_handler(m_raw_response_message.copy());
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

    void TearDown() override
    {
        m_thread_pool->stop();
    }

    std::unique_ptr<client_endpoint> create_sut()
    {
        return std::make_unique<client_endpoint>(m_service,
                                                 std::move(m_channel),
                                                 std::move(m_connector),
                                                 m_thread_pool,
                                                 m_connection_state_change_handler.AsStdFunction());
    }

    void emit_recv_event(bool res, buffer buff)
    {
        m_recv_handler(res, std::move(buff));
    }

protected:
    std::shared_ptr<thread_pool> m_thread_pool;

    const int m_stub_method = 1;
    const decltype(&proto::Service_Stub::Method2) m_stub_method_ptr = &proto::Service_Stub::Method2;
    response_result m_response_result = response_result::ok;

    proto::request_message m_request_message;
    proto::response_message m_response_message;
    buffer m_raw_request_message;
    buffer m_raw_response_message;
    MockFunction<void(buffer &&)> m_response_handler;

    std::shared_ptr<service_nice_mock> m_service;
    std::unique_ptr<channel_nice_mock> m_channel;
    std::unique_ptr<connector_nice_mock> m_connector;

    std::unique_ptr<transport_nice_mock> m_transport;
    transport_nice_mock *m_transport_ptr;

    std::function<void(bool, vshalygin::cl::buffer &&)> m_recv_handler;
    std::function<void()> m_stop_transport_handler;

    MockFunction<void(connection_state)> m_connection_state_change_handler;
};

TEST_F(ClientEndpoint, SetsRequestHandlerOnConstruction)
{
    event sync_event;
    EXPECT_CALL(*m_service, process_request)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    auto sut = create_sut();
    sut->connect();

    m_raw_request_message = create_transfer_msg_req(34, 1, &m_request_message);
    emit_recv_event(true, m_raw_request_message.copy());
    sync_event.wait();
}

TEST_F(ClientEndpoint, StartTransportAndSetsConnection)
{
    EXPECT_CALL(*m_connector, create_transport)
        .Times(1);
    EXPECT_CALL(*m_channel, set_connection)
        .Times(1);
    EXPECT_CALL(*m_transport_ptr, start)
        .Times(1);

    auto sut = create_sut();
    sut->connect();
}

TEST_F(ClientEndpoint, DisconnectsConnection)
{
    EXPECT_CALL(*m_transport_ptr, stop)
        .Times(1);

    auto sut = create_sut();
    sut->connect();
    sut->disconnect();

    Mock::VerifyAndClearExpectations(m_transport_ptr);
}

TEST_F(ClientEndpoint, ChecksConnection)
{
    EXPECT_CALL(*m_transport_ptr, is_running)
        .Times(1)
        .WillOnce(Return(true));

    auto sut = create_sut();
    sut->connect();
    const auto ans = sut->is_connected();

    ASSERT_TRUE(ans);
}

TEST_F(ClientEndpoint, SetsConnectionChangeStateHandler)
{
    auto sut = create_sut();

    EXPECT_CALL(m_connection_state_change_handler, Call(connection_state::connected))
        .Times(1);
    sut->connect();
    Mock::VerifyAndClearExpectations(&m_connection_state_change_handler);

    EXPECT_CALL(m_connection_state_change_handler, Call(connection_state::disconnected))
        .Times(1);
    sut->disconnect();
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
