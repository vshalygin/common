#include <rpc-lib/connector/connector.h>
#include <rpc-lib/pipe/memory-pipe/mem-pipe-env.h>
#include <rpc-lib/pipe/ipipe-endpoint.h>
#include <rpc-lib/connection/iconnection.h>
#include <rpc-lib/transfer-message/transfer-message.h>

#include <mocks/authenticator-mock.h>

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#pragma warning(pop)

#include <gtest/gtest.h>

using namespace vshalygin::rpc;
using namespace vshalygin::cl;
using namespace testing;
using namespace std::chrono_literals;

namespace {
    buffer create_req_message()
    {
        proto::null_message msg;
        return create_transfer_msg_req(34, 0, &msg);
    }
}

class Connector
    : public Test
{
protected:
    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(2);
        m_authenticator = std::make_shared<authenticator_nice_mock>();
        ON_CALL(*m_authenticator, check_response)
            .WillByDefault(Return(true));
        m_pipe_env = std::make_shared<mem_pipe_env>(m_thread_pool);
    }

    void TearDown() override
    {
        m_server_endpoint.reset();
        m_pipe_env.reset();
        m_authenticator.reset();
        m_thread_pool->stop();
    }

    auto create_sut(std::chrono::milliseconds send_timeout, std::chrono::milliseconds recv_timeout)
    {
        return std::make_unique<connector>(m_thread_pool, m_authenticator,
                                           m_pipe_env, nullptr, send_timeout, recv_timeout);
    }

    auto create_sut()
    {
        return create_sut(10s, 10s);
    }

    void create_server_pipe_end()
    {
        m_pipe_env->create_pipe()
            .then([&](pipe_wait_res, std::shared_ptr<ipipe_endpoint> p) {
                      m_server_endpoint = std::move(p);
                  })
            .get();
    }

protected:
    std::shared_ptr<thread_pool> m_thread_pool;
    std::shared_ptr<authenticator_nice_mock> m_authenticator;
    std::shared_ptr<mem_pipe_env> m_pipe_env;
    std::shared_ptr<ipipe_endpoint> m_server_endpoint;
};

TEST_F(Connector, CreatesConnection)
{
    auto sut = create_sut();
    auto f = sut->create_connection_async(10s);
    create_server_pipe_end();

    m_server_endpoint->read_async()
        .then([server_endpoint = m_server_endpoint](pipe_op_res , buffer &) {
                  return server_endpoint->write_async({});
              });

    std::unique_ptr<iconnection> con;
    f.get().apply([&](std::unique_ptr<iconnection> &&c) { con = std::move(c); });
    con->start();

    con->request_async(create_req_message());

    m_server_endpoint->read_async()
        .get()
        .apply([](pipe_op_res, buffer &b) { EXPECT_TRUE(b == create_req_message()); });
}

TEST_F(Connector, CancelWaitingConnection)
{
    auto sut = create_sut();
    auto f = sut->create_connection_async(10s);

    sut->cancel_connect_waiting();

    ASSERT_ANY_THROW(f.get());
}

TEST_F(Connector, FailsIfCannotWriteHandshakeRequest)
{
    auto sut = create_sut();
    auto f = sut->create_connection_async(10s);
    create_server_pipe_end();

    m_server_endpoint->invalidate();

    ASSERT_ANY_THROW(f.get());
}

TEST_F(Connector, FailsIfCannotWriteHandshakeRequestByTimout)
{
    auto sut = create_sut();
    auto f = sut->create_connection_async(0ms);
    create_server_pipe_end();

    ASSERT_ANY_THROW(f.get());
}

TEST_F(Connector, FailsIfCannotReadHandshakeResponse)
{
    auto sut = create_sut();
    auto f = sut->create_connection_async(10s);
    create_server_pipe_end();

    m_server_endpoint->read_async()
        .then([server_endpoint = m_server_endpoint](pipe_op_res, buffer &) {
                  server_endpoint->invalidate();
              });

    m_server_endpoint->invalidate();

    ASSERT_ANY_THROW(f.get());
}

TEST_F(Connector, FailsIfCannotReadHandshakeResponseByTimout)
{
    auto sut = create_sut();
    auto f = sut->create_connection_async(10ms);
    create_server_pipe_end();

    m_server_endpoint->read_async();

    ASSERT_ANY_THROW(f.get());
}

TEST_F(Connector, FailsIfConnectionRefusedByServer)
{
    EXPECT_CALL(*m_authenticator, check_response)
        .Times(1)
        .WillOnce(Return(false));

    auto sut = create_sut();
    auto f = sut->create_connection_async(10ms);
    create_server_pipe_end();

    m_server_endpoint->read_async()
        .then([server_endpoint = m_server_endpoint](pipe_op_res, buffer &) {
                  return server_endpoint->write_async({});
              });

    ASSERT_ANY_THROW(f.get());
}

TEST_F(Connector, UseAuthentificatorForFormingHandshakeRequest)
{
    buffer req(1); req[0] = std::byte(34);
    EXPECT_CALL(*m_authenticator, create_request)
        .Times(1)
        .WillOnce([&]() { return req.copy(); });

    auto sut = create_sut();
    auto f = sut->create_connection_async(10ms);
    create_server_pipe_end();

    m_server_endpoint->read_async()
        .then([&](pipe_op_res, buffer &buf) {
                  EXPECT_TRUE(req == buf);
              })
        .get();
}

TEST_F(Connector, UseAuthentificatorForCheckingHandshakeResponse)
{
    buffer res(1); res[0] = std::byte(34);
    EXPECT_CALL(*m_authenticator, check_response)
        .Times(1)
        .WillOnce([&](auto buf) { EXPECT_TRUE(buf == res); return true; });

    auto sut = create_sut();
    auto f = sut->create_connection_async(10ms);
    create_server_pipe_end();

    m_server_endpoint->read_async()
        .then([server_endpoint = m_server_endpoint, &res](pipe_op_res, buffer &) {
                  return server_endpoint->write_async(res.copy());
              })
        .get();

    f.get();
}

TEST_F(Connector, WaitingCancelsOnDestruction)
{
    auto sut = create_sut();
    auto f = sut->create_connection_async(10s);

    sut.reset();

    ASSERT_ANY_THROW(f.get());
}
