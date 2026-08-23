#include <rpc-lib/internal/connector/client-connector.h>
#include <rpc-lib/pipe/memory-pipe/mem-pipe-env.h>
#include <rpc-lib/pipe/ipipe-endpoint.h>
#include <rpc-lib/internal/connection/iconnection.h>
#include <rpc-lib/internal/transfer-message/transfer-message.h>

#include <mocks/authenticator-mock.h>

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#pragma warning(pop)

#include <gtest/gtest.h>

using namespace vshalygin::rpc;
using namespace vshalygin::rpc::internal;
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

class ClientConnector
    : public Test
{
protected:
    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(2);
        m_authenticator = std::make_shared<authenticator_nice_mock>();
        ON_CALL(*m_authenticator, check_response)
            .WillByDefault(Return(true));
        m_pipe_env = std::make_shared<mem_pipe_env>(m_thread_pool.get());
    }

    void TearDown() override
    {
        m_server_endpoint.reset();
        m_pipe_env.reset();
        m_authenticator.reset();
        m_thread_pool->stop();
    }

    auto create_sut(std::chrono::milliseconds handshake_timeout,
                    std::chrono::milliseconds send_timeout,
                    std::chrono::milliseconds recv_timeout)
    {
        config config;
        config.handshake_timeout = handshake_timeout;
        config.send_timeout = send_timeout;
        config.recv_timeout = recv_timeout;
        config.check_connection_period = std::chrono::seconds(10);
        config.ping_timeout = std::chrono::seconds(10);
        return std::make_unique<client_connector>(m_thread_pool.get(), m_authenticator,
                                                  m_pipe_env, config);
    }

    auto create_sut()
    {
        return create_sut(10s, 10s, 10s);
    }

    void create_server_pipe_end()
    {
        m_pipe_env->create_pipe(0)
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

TEST_F(ClientConnector, CreatesConnection)
{
    auto sut = create_sut();
    auto f = sut->create_connection_async(nullptr, 10s);
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

TEST_F(ClientConnector, FailsIfCannotWriteHandshakeRequest)
{
    auto sut = create_sut();
    auto f = sut->create_connection_async(nullptr, 10s);
    create_server_pipe_end();

    m_server_endpoint->invalidate();

    ASSERT_ANY_THROW(f.get());
}

TEST_F(ClientConnector, FailsIfCannotWriteHandshakeRequestByTimout)
{
    auto sut = create_sut(0ms, 10s, 10s);
    auto f = sut->create_connection_async(nullptr, 10s);
    create_server_pipe_end();

    ASSERT_ANY_THROW(f.get());
}

TEST_F(ClientConnector, FailsIfCannotReadHandshakeResponse)
{
    auto sut = create_sut();
    auto f = sut->create_connection_async(nullptr, 10s);
    create_server_pipe_end();

    m_server_endpoint->read_async()
        .then([server_endpoint = m_server_endpoint](pipe_op_res, buffer &) {
                  server_endpoint->invalidate();
              });

    m_server_endpoint->invalidate();

    ASSERT_ANY_THROW(f.get());
}

TEST_F(ClientConnector, FailsIfCannotReadHandshakeResponseByTimout)
{
    auto sut = create_sut(10ms, 10s, 10s);
    auto f = sut->create_connection_async(nullptr, 10s);
    create_server_pipe_end();

    m_server_endpoint->read_async();

    ASSERT_ANY_THROW(f.get());
}

TEST_F(ClientConnector, FailsIfConnectionRefusedByServer)
{
    EXPECT_CALL(*m_authenticator, check_response)
        .Times(1)
        .WillOnce(Return(false));

    auto sut = create_sut(10ms, 10s, 10s);
    auto f = sut->create_connection_async(nullptr, 10s);
    create_server_pipe_end();

    m_server_endpoint->read_async()
        .then([server_endpoint = m_server_endpoint](pipe_op_res, buffer &) {
                  return server_endpoint->write_async({});
              });

    ASSERT_ANY_THROW(f.get());
}

TEST_F(ClientConnector, UseAuthentificatorForFormingHandshakeRequest)
{
    buffer req(1); req[0] = std::byte(34);
    EXPECT_CALL(*m_authenticator, create_request)
        .Times(1)
        .WillOnce([&]() { return req.copy(); });

    auto sut = create_sut(10ms, 10s, 10s);
    auto f = sut->create_connection_async(nullptr, 10s);
    create_server_pipe_end();

    m_server_endpoint->read_async()
        .then([&](pipe_op_res, buffer &buf) {
                  EXPECT_TRUE(req == buf);
              })
        .get();
}

TEST_F(ClientConnector, UseAuthentificatorForCheckingHandshakeResponse)
{
    buffer res(1); res[0] = std::byte(34);
    EXPECT_CALL(*m_authenticator, check_response)
        .Times(1)
        .WillOnce([&](auto buf) { EXPECT_TRUE(buf == res); return true; });

    auto sut = create_sut(10ms, 10s, 10s);
    auto f = sut->create_connection_async(nullptr, 10s);
    create_server_pipe_end();

    m_server_endpoint->read_async()
        .then([server_endpoint = m_server_endpoint, &res](pipe_op_res, buffer &) {
                  return server_endpoint->write_async(res.copy());
              })
        .get();

    f.get();
}

TEST_F(ClientConnector, WaitingCancelsOnDestruction)
{
    auto sut = create_sut();
    auto f = sut->create_connection_async(nullptr, 1000s);

    sut.reset();

    ASSERT_ANY_THROW(f.get());
}

TEST_F(ClientConnector, WaitingPipeCancelsOnTimeout)
{
    auto sut = create_sut();
    auto f = sut->create_connection_async(nullptr, 1ms);

    ASSERT_ANY_THROW(f.get());
}
