#include <rpc-lib/server-endpoint/server-endpoint.h>
#include <rpc-lib/transfer-message/transfer-message.h>
#include <common-lib/syncronization/event/event.h>

#include "mocks/service-mock.h"
#include "mocks/listener-mock.h"
#include "mocks/transport-mock.h"

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#include <google/protobuf/util/message_differencer.h>
#pragma warning(pop)

#include <common-lib/thread-pool/thread-pool.h>
#include <gtest/gtest.h>

using namespace vshalygin::rpc;
using namespace vshalygin::cl;
using namespace testing;
using namespace google::protobuf::util;

class ServerEndpoint
    : public Test
{
protected:
    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(1);
        m_listener = std::make_unique<listener_nice_mock>();
        m_service = std::make_shared<service_nice_mock>();

        m_req_message.set_data(34);
        m_res_message.set_data(45);

        m_create_stub = [](google::protobuf::RpcChannel *channel) { return proto::Service_Stub(channel);  };
    }

    std::unique_ptr<server_endpoint> create_sut()
    {
        return std::make_unique<server_endpoint>(std::move(m_listener),
                                                 m_service,
                                                 m_thread_pool,
                                                 m_connection_change_state_handler.AsStdFunction());
    }

protected:
    MockFunction<void(uint64_t, connection_state)> m_connection_change_state_handler;

    proto::request_message m_req_message;
    proto::response_message m_res_message;
    std::shared_ptr<thread_pool> m_thread_pool;
    std::unique_ptr<listener_nice_mock> m_listener;
    std::shared_ptr<service_nice_mock> m_service;

    std::function<proto::Service_Stub(google::protobuf::RpcChannel *ch)> m_create_stub;
};

TEST_F(ServerEndpoint, SetsConnectHandlerOnConstruction)
{
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1);

    create_sut();
}

TEST_F(ServerEndpoint, StartsTransportOnNewCollection)
{
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));

    auto transport = std::make_unique<transport_nice_mock>();
    EXPECT_CALL(*transport, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto &&) { start_callback(); });

    auto sut = create_sut();
    new_connection_handler(std::move(transport));
}

TEST_F(ServerEndpoint, AnswersInactiveConnectionCountAsZeroIfTranportStartThrowsException)
{
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));

    auto transport = std::make_unique<transport_nice_mock>();
    EXPECT_CALL(*transport, start)
        .Times(1)
        .WillOnce(Throw(std::exception()));

    auto sut = create_sut();
    new_connection_handler(std::move(transport));

    ASSERT_EQ(sut->get_inactive_connections_count(), 0);
}

TEST_F(ServerEndpoint, AnswersInactiveConnectionCountAsZeroIfTranportStartCallbackCalled)
{
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));

    auto transport = std::make_unique<transport_nice_mock>();
    EXPECT_CALL(*transport, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto &&) { start_callback(); });

    auto sut = create_sut();
    new_connection_handler(std::move(transport));

    ASSERT_EQ(sut->get_inactive_connections_count(), 0);
}

TEST_F(ServerEndpoint, AnswersZeroChannelsIfNewConnectionHandlerWasNotCalled)
{
    auto sut = create_sut();

    ASSERT_EQ(sut->get_channels_count(), 0);
}

TEST_F(ServerEndpoint, AnswersOneChannelCountIfNewConnectionHandlerWasCalledAndTransportStartHandlerWasCalled)
{
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));

    auto transport = std::make_unique<transport_nice_mock>();
    EXPECT_CALL(*transport, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto &&) { start_callback(); });

    auto sut = create_sut();
    new_connection_handler(std::move(transport));

    ASSERT_EQ(sut->get_channels_count(), 1);
}

TEST_F(ServerEndpoint, CallsSetConnectionChangeHandler)
{
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));
    auto transport = std::make_unique<transport_nice_mock>();
    std::function<void()> transport_started_callback;
    EXPECT_CALL(*transport, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto &&) { start_callback(); });

    EXPECT_CALL(m_connection_change_state_handler, Call(_, connection_state::connected))
        .Times(1);

    auto sut = create_sut();
    new_connection_handler(std::move(transport));
}

TEST_F(ServerEndpoint, CallsConnectionChangeHandlerSeveralTimesWithVariousIds)
{
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));
    auto transport1 = std::make_unique<transport_nice_mock>();
    auto transport2 = std::make_unique<transport_nice_mock>();
    auto transport3 = std::make_unique<transport_nice_mock>();
    EXPECT_CALL(*transport1, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto &&) { start_callback(); });
    EXPECT_CALL(*transport2, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto &&) { start_callback(); });
    EXPECT_CALL(*transport3, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto &&) { start_callback(); });
    EXPECT_CALL(m_connection_change_state_handler, Call(0, _))
        .Times(1);
    EXPECT_CALL(m_connection_change_state_handler, Call(1, _))
        .Times(1);
    EXPECT_CALL(m_connection_change_state_handler, Call(2, _))
        .Times(1);

    auto sut = create_sut();
    new_connection_handler(std::move(transport1));
    new_connection_handler(std::move(transport2));
    new_connection_handler(std::move(transport3));
}

TEST_F(ServerEndpoint, AnswersZeroChannelCountIfTransportWasStopped)
{
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));
    auto transport = std::make_unique<transport_nice_mock>();
    auto transport_ptr = transport.get();
    std::function<void()> transport_started_callback;
    std::function<void()> transport_stop_callback;
    EXPECT_CALL(*transport, start)
        .Times(1)
        .WillOnce([&](auto &&start_callback, auto &&stop_callback) {
                      start_callback();
                      transport_started_callback = std::move(start_callback);
                      transport_stop_callback = std::move(stop_callback);
                  });
    EXPECT_CALL(*transport, stop)
        .Times(2)
        .WillOnce([&]() { transport_stop_callback(); })
        .WillRepeatedly(DoDefault());
    EXPECT_CALL(m_connection_change_state_handler, Call)
        .Times(2);

    auto sut = create_sut();
    new_connection_handler(std::move(transport));
    ASSERT_EQ(sut->get_channels_count(), 1);

    transport_ptr->stop();
    ASSERT_EQ(sut->get_channels_count(), 0);
}

TEST_F(ServerEndpoint, ProcessesRequestsFromClients)
{
    event sync_event;
    buffer req_buffer = create_transfer_msg_req(3, 0, &m_req_message);
    buffer res_buffer = create_transfer_msg_res(3, response_result::ok, &m_res_message);
    EXPECT_CALL(*m_service, process_request)
        .Times(1)
        .WillOnce([&](auto &&buf, auto &&callback) {
                      EXPECT_EQ(req_buffer, buf);
                      callback(res_buffer.copy());
                  });

    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));
    std::function<void(bool, vshalygin::cl::buffer &&)> recv_handler;
    auto transport = std::make_unique<transport_nice_mock>();
    EXPECT_CALL(*transport, recv_async)
        .Times(AtLeast(1))
        .WillOnce(SaveArg<0>(&recv_handler));
    EXPECT_CALL(*transport, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto) { start_callback(); });
    EXPECT_CALL(*transport, send_async)
        .Times(1)
        .WillOnce([&](auto &&buf, auto) {
                      EXPECT_EQ(res_buffer, buf);
                      sync_event.set();
                  });

    auto sut = create_sut();
    new_connection_handler(std::move(transport));
 
    recv_handler(true, req_buffer.copy());
    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(ServerEndpoint, StartsListening)
{
    EXPECT_CALL(*m_listener, start)
        .Times(1);

    auto sut = create_sut();
    sut->start_listen();
}

TEST_F(ServerEndpoint, StopsListening)
{
    EXPECT_CALL(*m_listener, stop)
        .Times(1);

    auto sut = create_sut();
    sut->stop_listen();
}

TEST_F(ServerEndpoint, ChecksListening)
{
    EXPECT_CALL(*m_listener, is_stopped)
        .Times(1)
        .WillOnce(Return(false));

    auto sut = create_sut();
    auto res = sut->is_listening();

    ASSERT_TRUE(res);
}

TEST_F(ServerEndpoint, SetListenerChangeStateHandler)
{
    MockFunction<void(listener_state)> listener_change_state_handler;
    EXPECT_CALL(listener_change_state_handler, Call(listener_state::stopped))
        .Times(1);
    EXPECT_CALL(*m_listener, set_change_state_handler)
        .Times(1)
        .WillOnce([](auto &&handler) { handler(listener_state::stopped); });

    auto sut = create_sut();
    sut->set_listener_change_state_handler(listener_change_state_handler.AsStdFunction());
}

TEST_F(ServerEndpoint, DropsConnectionWithId)
{
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));
    auto transport1 = std::make_unique<transport_nice_mock>();
    auto transport1_ptr = transport1.get();
    auto transport2 = std::make_unique<transport_nice_mock>();
    auto transport2_ptr = transport2.get();
    EXPECT_CALL(*transport1, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto) { start_callback(); });
    EXPECT_CALL(*transport1, stop)
        .Times(1);
    EXPECT_CALL(*transport2, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto) { start_callback(); });
    EXPECT_CALL(*transport2, stop)
        .Times(0);
    EXPECT_CALL(m_connection_change_state_handler, Call(0, _))
        .Times(1);
    EXPECT_CALL(m_connection_change_state_handler, Call(1, _))
        .Times(1);
    auto sut = create_sut();
    new_connection_handler(std::move(transport1));
    new_connection_handler(std::move(transport2));

    sut->drop_connection(0);
    Mock::VerifyAndClearExpectations(transport1_ptr);
    Mock::VerifyAndClearExpectations(transport2_ptr);
}

TEST_F(ServerEndpoint, DropsAllConnections)
{
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));
    auto transport1 = std::make_unique<transport_nice_mock>();
    auto transport1_ptr = transport1.get();
    auto transport2 = std::make_unique<transport_nice_mock>();
    auto transport2_ptr = transport2.get();
    EXPECT_CALL(*transport1, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto) { start_callback(); });
    EXPECT_CALL(*transport1, stop)
        .Times(1);
    EXPECT_CALL(*transport2, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto) { start_callback(); });
    EXPECT_CALL(*transport2, stop)
        .Times(1);
    EXPECT_CALL(m_connection_change_state_handler, Call(0, _))
        .Times(1);
    EXPECT_CALL(m_connection_change_state_handler, Call(1, _))
        .Times(1);
    auto sut = create_sut();
    new_connection_handler(std::move(transport1));
    new_connection_handler(std::move(transport2));

    sut->drop_all_connections();
    Mock::VerifyAndClearExpectations(transport1_ptr);
    Mock::VerifyAndClearExpectations(transport2_ptr);
}

TEST_F(ServerEndpoint, ThrowsExceptionOnAttemptToMakeRequestWithUnexistingId)
{
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));
    auto transport = std::make_unique<transport_nice_mock>();
    EXPECT_CALL(*transport, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto) { start_callback(); });
    EXPECT_CALL(m_connection_change_state_handler, Call(_, connection_state::connected))
        .Times(1);

    auto sut = create_sut();
    new_connection_handler(std::move(transport));

    try {
        sut->make_request<proto::request_message, proto::response_message>(1000,
                                                                           m_req_message,
                                                                           m_create_stub,
                                                                           &proto::Service_Stub::Method);
        FAIL();
    } catch(...){}
}

TEST_F(ServerEndpoint, MakeSyncRequestOnSpecifiedConnection)
{
    const auto connection_id = 0;
    buffer req_buffer = create_transfer_msg_req(0, 1, &m_req_message);
    buffer res_buffer = create_transfer_msg_res(0, response_result::ok, &m_res_message);
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));
    std::function<void(bool, vshalygin::cl::buffer &&)> recv_handler;
    auto transport = std::make_unique<transport_nice_mock>();
    EXPECT_CALL(*transport, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto) { start_callback(); });
    EXPECT_CALL(*transport, recv_async)
        .Times(AtLeast(1))
        .WillOnce(SaveArg<0>(&recv_handler));
    EXPECT_CALL(*transport, send_async)
        .Times(1)
        .WillOnce([&](auto &&buf, auto &&) {
                      EXPECT_EQ(buf, req_buffer);
                      recv_handler(true, res_buffer.copy());
                  });

    EXPECT_CALL(m_connection_change_state_handler, Call(connection_id, _))
        .Times(1);
    auto sut = create_sut();
    new_connection_handler(std::move(transport));

    auto res = sut->make_request<proto::request_message, proto::response_message>
        (connection_id, m_req_message, m_create_stub, &proto::Service_Stub::Method2);

    ASSERT_TRUE(MessageDifferencer::Equals(m_res_message, *res));
}

TEST_F(ServerEndpoint, ThrowsExceptionIfMakeSyncRequestOnSpecifiedConnectionFailedByResponseCode)
{
    const unsigned connection_id = 0;
    buffer req_buffer = create_transfer_msg_req(0, 1, &m_req_message);
    buffer res_buffer = create_transfer_msg_res(0, response_result::not_implemented, &m_res_message);
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));
    std::function<void(bool, vshalygin::cl::buffer &&)> recv_handler1;
    auto transport1 = std::make_unique<transport_nice_mock>();
    EXPECT_CALL(*transport1, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto) { start_callback(); });
    EXPECT_CALL(*transport1, recv_async)
        .Times(AtLeast(1))
        .WillOnce(SaveArg<0>(&recv_handler1));
    EXPECT_CALL(*transport1, send_async)
        .Times(1)
        .WillOnce([&](auto &&buf, auto &&) {
                      EXPECT_EQ(buf, req_buffer);
                      recv_handler1(true, res_buffer.copy());
                  });

    EXPECT_CALL(m_connection_change_state_handler, Call(connection_id, _))
        .Times(1);
    auto sut = create_sut();
    new_connection_handler(std::move(transport1));

    try {
        sut->make_request<proto::request_message, proto::response_message>
            (connection_id, m_req_message, m_create_stub, &proto::Service_Stub::Method2);
        FAIL();
    } catch(const request_exception &e) {
        ASSERT_EQ(e.code(), request_result::request_not_processed);
    } catch (...) {
        FAIL();
    }
}

TEST_F(ServerEndpoint, MakesSyncRequestToAllConnections)
{
    const auto connection_id = 0;
    buffer req_buffer = create_transfer_msg_req(0, 1, &m_req_message);
    buffer res_buffer = create_transfer_msg_res(0, response_result::ok, &m_res_message);
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));
    std::function<void(bool, vshalygin::cl::buffer &&)> recv_handler;
    auto transport = std::make_unique<transport_nice_mock>();
    EXPECT_CALL(*transport, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto) { start_callback(); });
    EXPECT_CALL(*transport, recv_async)
        .Times(AtLeast(1))
        .WillOnce(SaveArg<0>(&recv_handler));
    EXPECT_CALL(*transport, send_async)
        .Times(1)
        .WillOnce([&](auto &&buf, auto &&) {
                      EXPECT_EQ(buf, req_buffer);
                      recv_handler(true, res_buffer.copy());
                  });

    EXPECT_CALL(m_connection_change_state_handler, Call(connection_id, _))
        .Times(1);
    auto sut = create_sut();
    new_connection_handler(std::move(transport));

    auto res = sut->make_request_all<proto::request_message, proto::response_message>
        (m_req_message, m_create_stub, &proto::Service_Stub::Method2);

    ASSERT_EQ(res.size(), 1);
    ASSERT_EQ(res[0].first, request_result::ok);
    ASSERT_TRUE(MessageDifferencer::Equals(m_res_message, *(res[0].second)));
}

TEST_F(ServerEndpoint, ThrowsExceptionOnAttenptToMakeRequestAsyncWithUnexistingId)
{
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));
    auto transport = std::make_unique<transport_nice_mock>();
    EXPECT_CALL(*transport, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto) { start_callback(); });
    EXPECT_CALL(m_connection_change_state_handler, Call(_, connection_state::connected))
        .Times(1);

    auto sut = create_sut();
    new_connection_handler(std::move(transport));

    try {
        sut->make_request_async<proto::request_message, proto::response_message>(1000,
                                                                                 m_req_message,
                                                                                 m_create_stub,
                                                                                 &proto::Service_Stub::Method,
                                                                                 {});
        FAIL();
    }
    catch(...) {}
}

TEST_F(ServerEndpoint, MakeAsyncRequestOnSpecifiedConnection)
{
    const auto connection_id = 0;
    event sync_event;
    auto request_callback = [&](uint64_t id,
                                request_result requst_res,
                                std::unique_ptr<proto::response_message> res) {
        EXPECT_EQ(id, connection_id);
        EXPECT_EQ(requst_res, request_result::ok);
        EXPECT_TRUE(MessageDifferencer::Equals(*res, m_res_message));
        sync_event.set();
    };
    buffer req_buffer = create_transfer_msg_req(0, 1, &m_req_message);
    buffer res_buffer = create_transfer_msg_res(0, response_result::ok, &m_res_message);
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));
    std::function<void(bool, vshalygin::cl::buffer &&)> recv_handler;
    auto transport = std::make_unique<transport_nice_mock>();
    std::function<void()> transport_started_callback;
    EXPECT_CALL(*transport, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto) { start_callback(); });
    EXPECT_CALL(*transport, recv_async)
        .Times(AtLeast(1))
        .WillOnce(SaveArg<0>(&recv_handler));
    EXPECT_CALL(*transport, send_async)
        .Times(1)
        .WillOnce([&](auto &&buf, auto &&) {
                      EXPECT_EQ(buf, req_buffer);
                      recv_handler(true, res_buffer.copy());
                  });

    EXPECT_CALL(m_connection_change_state_handler, Call(connection_id, _))
        .Times(1);
    auto sut = create_sut();
    new_connection_handler(std::move(transport));

    sut->make_request_async<proto::request_message, proto::response_message>
        (connection_id, m_req_message, m_create_stub, &proto::Service_Stub::Method2, request_callback);

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(ServerEndpoint, MakeUnsuccessfulAsyncRequestOnSpecifiedConnection)
{
    const auto connection_id = 0;
    event sync_event;
    auto request_callback = [&](uint64_t id,
                                request_result requst_res,
                                std::unique_ptr<proto::response_message>) {
        EXPECT_EQ(id, connection_id);
        EXPECT_EQ(requst_res, request_result::request_not_processed);
        sync_event.set();
    };
    buffer req_buffer = create_transfer_msg_req(0, 1, &m_req_message);
    buffer res_buffer = create_transfer_msg_res(0, response_result::not_implemented, &m_res_message);
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));
    std::function<void(bool, vshalygin::cl::buffer &&)> recv_handler1;
    auto transport1 = std::make_unique<transport_nice_mock>();
    EXPECT_CALL(*transport1, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto) { start_callback(); });
    EXPECT_CALL(*transport1, recv_async)
        .Times(AtLeast(1))
        .WillOnce(SaveArg<0>(&recv_handler1));
    EXPECT_CALL(*transport1, send_async)
        .Times(1)
        .WillOnce([&](auto &&buf, auto &&) {
                      EXPECT_EQ(buf, req_buffer);
                      recv_handler1(true, res_buffer.copy());
                  });

    EXPECT_CALL(m_connection_change_state_handler, Call(connection_id, _))
        .Times(1);
    auto sut = create_sut();
    new_connection_handler(std::move(transport1));

    sut->make_request_async<proto::request_message, proto::response_message>
        (connection_id, m_req_message, m_create_stub, &proto::Service_Stub::Method2, request_callback);

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(ServerEndpoint, MakesAsyncRequestToAllConnections)
{
    const auto connection_id = 0;
    event sync_event;
    auto request_callback = [&](uint64_t id,
                                request_result requst_res,
                                std::unique_ptr<proto::response_message> res) {
        EXPECT_EQ(id, connection_id);
        EXPECT_EQ(requst_res, request_result::ok);
        EXPECT_TRUE(MessageDifferencer::Equals(*res, m_res_message));
        sync_event.set();
    };
    buffer req_buffer = create_transfer_msg_req(0, 1, &m_req_message);
    buffer res_buffer = create_transfer_msg_res(0, response_result::ok, &m_res_message);
    std::function<void(std::unique_ptr<itransport>)> new_connection_handler;
    EXPECT_CALL(*m_listener, set_connect_handler)
        .Times(1)
        .WillOnce(SaveArg<0>(&new_connection_handler));
    std::function<void(bool, vshalygin::cl::buffer &&)> recv_handler;
    auto transport = std::make_unique<transport_nice_mock>();
    EXPECT_CALL(*transport, start)
        .Times(1)
        .WillOnce([](auto &&start_callback, auto) { start_callback(); });
    EXPECT_CALL(*transport, recv_async)
        .Times(AtLeast(1))
        .WillOnce(SaveArg<0>(&recv_handler));
    EXPECT_CALL(*transport, send_async)
        .Times(1)
        .WillOnce([&](auto &&buf, auto &&) {
        EXPECT_EQ(buf, req_buffer);
        recv_handler(true, res_buffer.copy());
    });

    EXPECT_CALL(m_connection_change_state_handler, Call(connection_id, _))
        .Times(1);
    auto sut = create_sut();
    new_connection_handler(std::move(transport));

    auto num = sut->make_request_all_async<proto::request_message, proto::response_message>
        (m_req_message, m_create_stub, &proto::Service_Stub::Method2, request_callback);

    EXPECT_EQ(num, 1);
    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}
