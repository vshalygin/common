#include <rpc-lib/internal/endpoint/endpoint.h>
#include <rpc-lib/internal/transfer-message/transfer-message.h>

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/synchronization/event/event.h>

#include <mocks/connection-mock.h>

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#pragma warning(pop)

#include <gtest/gtest.h>

using namespace vshalygin;
using namespace vshalygin::rpc;
using namespace vshalygin::rpc::internal;
using namespace vshalygin::cl;
using namespace testing;

class Endpoint
    : public Test
{
protected:
    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(2);
        auto connection_mock = std::make_unique<connection_nice_mock>();
        m_connection_mock = connection_mock.get();

        m_sut = std::make_unique<endpoint<proto::Service_Stub>>(std::move(connection_mock), m_thread_pool);
    }

    void TearDown() override
    {
        m_sut.reset();
        m_thread_pool->stop();
    }

protected:
    std::shared_ptr<thread_pool> m_thread_pool;
    connection_nice_mock *m_connection_mock;
    std::unique_ptr<endpoint<proto::Service_Stub>> m_sut;
};

TEST_F(Endpoint, TestIsConnected)
{
    EXPECT_CALL(*m_connection_mock, is_active)
        .Times(1)
        .WillOnce(Return(true));

    ASSERT_TRUE(m_sut->is_connected());
}

TEST_F(Endpoint, TestDisconnect)
{
    EXPECT_CALL(*m_connection_mock, deactivate)
        .Times(1);

    m_sut->disconnect();
}

TEST_F(Endpoint, TestStart)
{
    EXPECT_CALL(*m_connection_mock, start)
        .Times(1);

    m_sut->start();
}

TEST_F(Endpoint, TestSetDisconnectHandler)
{
    event sync_event;
    MockFunction<void()> disconnect_callback;
    EXPECT_CALL(disconnect_callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    EXPECT_CALL(*m_connection_mock, set_stop_callback)
        .Times(1)
        .WillOnce([&](auto &&f) { m_thread_pool->post(std::move(f)); });

    m_sut->set_disconnect_callback(disconnect_callback.AsStdFunction());
    sync_event.wait();
}

TEST_F(Endpoint, TestMakeRequest)
{
    auto promise = make_promise(m_thread_pool.get(), [](request_result r, buffer &&b) {
        return rpc::ftuple(r, std::move(b));
    });

    proto::request_message request;
    request.set_data(34);

    proto::response_message response;
    response.set_data(34);

    auto expected_req_message = create_transfer_msg_req(0, 0, &request);
    auto response_message = create_transfer_msg_res(0, response_result::ok, &response);

    event sync_event;
    EXPECT_CALL(*m_connection_mock, request_async)
        .Times(1)
        .WillOnce([&](buffer &&b) {
                      EXPECT_TRUE(b == expected_req_message);
                      promise.resolve(request_result::ok, std::move(response_message));
                      sync_event.set();
                      return promise.get_future();
                  });

    m_sut->make_request<proto::request_message, proto::response_message>(&proto::Service_Stub::Method, request);

    sync_event.wait();
}
