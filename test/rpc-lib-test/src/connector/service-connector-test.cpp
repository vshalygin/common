#include <rpc-lib/internal/connector/server-connector.h>
#include <rpc-lib/pipe/memory-pipe/mem-pipe-env.h>
#include <rpc-lib/internal/connection/iconnection.h>
#include <rpc-lib/pipe/ipipe-endpoint.h>

#include <mocks/authenticator-mock.h>
#include <mocks/service-mock.h>

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/synchronization/event/event.h>

#include <gtest/gtest.h>

using namespace vshalygin::rpc;
using namespace vshalygin::rpc::internal;
using namespace vshalygin::cl;
using namespace testing;

class ServiceConnector
    : public Test
{
protected:
    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(2);

        ON_CALL(m_on_new_connection, Call)
            .WillByDefault([]() {});
        ON_CALL(m_on_state_change, Call)
            .WillByDefault([]() {});

        m_authenticator = std::make_shared<authenticator_nice_mock>();
        m_mem_pipe_env = std::make_shared<mem_pipe_env>(m_thread_pool);
        m_service = std::make_unique<service_nice_mock>();
        m_service2 = std::make_unique<service_nice_mock>();

        ON_CALL(*m_authenticator, check_request)
            .WillByDefault(Return(true));
    }

    void TearDown() override
    {
        m_mem_pipe_env.reset();
        m_thread_pool->stop();
    }

    auto create_sut()
    {
        return std::make_unique<server_connector>(m_thread_pool,
                                                  m_authenticator,
                                                  m_mem_pipe_env,
                                                  [this](uint64_t) {
                                                      if(m_service) {
                                                          return std::move(m_service);
                                                      } else if (m_service2) {
                                                          return std::move(m_service2);
                                                      } else {
                                                          assert(false);
                                                          throw;
                                                      }
                                                  },
                                                  m_on_new_connection.AsStdFunction(),
                                                  m_on_state_change.AsStdFunction(),
                                                  m_handshake_timeout,
                                                  m_send_timeout,
                                                  m_recv_timeout);
    }

protected:
    MockFunction<void(uint64_t, std::unique_ptr<iconnection>)> m_on_new_connection;
    MockFunction<void(server_connector_state)> m_on_state_change;

    std::shared_ptr<authenticator_nice_mock> m_authenticator;

    std::unique_ptr<service_nice_mock> m_service;
    std::unique_ptr<service_nice_mock> m_service2;

    std::shared_ptr<mem_pipe_env> m_mem_pipe_env;

    std::chrono::milliseconds m_handshake_timeout{10000};
    std::chrono::milliseconds m_send_timeout{ 10000 };
    std::chrono::milliseconds m_recv_timeout{ 10000 };

    std::shared_ptr<thread_pool> m_thread_pool;
};

TEST_F(ServiceConnector, InitiallyNotActive)
{
    auto sut = create_sut();

    ASSERT_FALSE(sut->is_active());
}

TEST_F(ServiceConnector, ActiveAfterStart)
{
    auto sut = create_sut();
    sut->start();

    ASSERT_TRUE(sut->is_active());
}

TEST_F(ServiceConnector, NotActiveAfterStop)
{
    auto sut = create_sut();
    sut->start();
    sut->stop();

    ASSERT_FALSE(sut->is_active());
}

TEST_F(ServiceConnector, ExecutesStartCallbacksOnStart)
{
    auto sync_event = std::make_shared<event>();
    EXPECT_CALL(m_on_state_change, Call(server_connector_state::started))
        .Times(1);
    EXPECT_CALL(m_on_state_change, Call(server_connector_state::stopped))
        .Times(1)
        .WillOnce([sync_event]() { sync_event->set(); });

    auto sut = create_sut();
    sut->start();

    sut.reset();
    sync_event->wait();
}

TEST_F(ServiceConnector, ExecutesStopCallbacksOnStop)
{
    auto sync_event = std::make_shared<event>();
    EXPECT_CALL(m_on_state_change, Call(server_connector_state::started))
        .Times(1);
    EXPECT_CALL(m_on_state_change, Call(server_connector_state::stopped))
        .Times(1)
        .WillOnce([sync_event]() { sync_event->set(); });

    auto sut = create_sut();
    sut->start();
    sut->stop();

    sync_event->wait();
}

TEST_F(ServiceConnector, CreatesTwoConnections)
{
    auto sync_event = std::make_shared<event>();
    EXPECT_CALL(m_on_new_connection, Call)
        .Times(2)
        .WillOnce(DoDefault())
        .WillOnce([sync_event]() {
                      sync_event->set();
                  });

    auto sut = create_sut();
    sut->start();

    std::shared_ptr<ipipe_endpoint> pe1;
    std::shared_ptr<ipipe_endpoint> pe2;
    m_mem_pipe_env->open_pipe()
        .then([&](pipe_wait_res, std::shared_ptr<ipipe_endpoint> pe) {
                  pe->write_async({});
                  pe->read_async();
                  pe1 = pe;
              });
    m_mem_pipe_env->open_pipe()
        .then([&](pipe_wait_res, std::shared_ptr<ipipe_endpoint> pe) {
                  pe->write_async({});
                  pe->read_async();
                  pe2 = pe;
              });

    sync_event->wait();
}

TEST_F(ServiceConnector, StopsIfPipeConnectionFailed)
{
    auto sync_event = std::make_shared<event>();
    EXPECT_CALL(m_on_state_change, Call(server_connector_state::started))
        .Times(1);
    EXPECT_CALL(m_on_state_change, Call(server_connector_state::stopped))
        .Times(1)
        .WillOnce([sync_event]() { sync_event->set(); });

    auto sut = create_sut();
    sut->start();

    while(m_mem_pipe_env->get_pending_server_endpoints_count() == 0) {}
    m_mem_pipe_env->cancel_pending_server_endpoints();

    sync_event->wait();
    ASSERT_FALSE(sut->is_active());
}

TEST_F(ServiceConnector, DoesNotCreateConnectionIfHandshakeReadFailed)
{
    EXPECT_CALL(m_on_new_connection, Call)
        .Times(0);

    m_handshake_timeout = std::chrono::milliseconds(0);

    auto sut = create_sut();
    sut->start();

    m_mem_pipe_env->open_pipe();

    m_mem_pipe_env->open_pipe().get();
}

TEST_F(ServiceConnector, DoesNotCreateConnectionIfAuthenticationFailed)
{
    auto sync_event = std::make_shared<event>();
    EXPECT_CALL(m_on_new_connection, Call)
        .Times(0);
    EXPECT_CALL(*m_authenticator, check_request)
        .Times(1)
        .WillOnce([sync_event]() { sync_event->set(); return false; });

    auto sut = create_sut();
    sut->start();

    auto f = m_mem_pipe_env->open_pipe();
    f.get().apply([](pipe_wait_res, std::shared_ptr<ipipe_endpoint> pe) {
        pe->write_async({});
    });

    m_mem_pipe_env->open_pipe().get();
    sync_event->wait();
}

TEST_F(ServiceConnector, DoesNotCreateConnectionIfWriteFailed)
{
    m_handshake_timeout = std::chrono::milliseconds(10);

    auto sync_event = std::make_shared<event>();
    EXPECT_CALL(m_on_new_connection, Call)
        .Times(0);
    EXPECT_CALL(*m_authenticator, check_request)
        .Times(1)
        .WillOnce([sync_event]() { sync_event->set(); return false; });

    auto sut = create_sut();
    sut->start();

    auto f = m_mem_pipe_env->open_pipe();
    f.get().apply([](pipe_wait_res, std::shared_ptr<ipipe_endpoint> pe) {
        pe->write_async({});
    });

    m_mem_pipe_env->open_pipe().get();
    sync_event->wait();
}

TEST_F(ServiceConnector, ExecuteStopCallbackOnDestruction)
{
    auto sync_event = std::make_shared<event>();
    EXPECT_CALL(m_on_state_change, Call(server_connector_state::started))
        .Times(1);
    EXPECT_CALL(m_on_state_change, Call(server_connector_state::stopped))
        .Times(1)
        .WillOnce([sync_event]() { sync_event->set(); });

    auto sut = create_sut();
    sut->start();

    sut.reset();

    sync_event->wait();
}

TEST_F(ServiceConnector, MayStartAfterStop)
{
    auto sut = create_sut();
    sut->start();
    auto sync_event = std::make_shared<event>();
    EXPECT_CALL(m_on_new_connection, Call)
        .Times(1)
        .WillOnce([sync_event]() { sync_event->set(); });
    std::shared_ptr<ipipe_endpoint> pe1;
    m_mem_pipe_env->open_pipe()
        .then([&](pipe_wait_res, std::shared_ptr<ipipe_endpoint> pe) {
                  pe->write_async({});
                  pe->read_async();
                  pe1 = pe;
              });
    sync_event->wait();
    sut->stop();
    while(sut->is_active()) {}

    Mock::VerifyAndClearExpectations(&m_on_new_connection);
    sync_event->reset();
    EXPECT_CALL(m_on_new_connection, Call)
        .Times(1)
        .WillOnce([sync_event]() { sync_event->set(); });

    sut->start();
    std::shared_ptr<ipipe_endpoint> pe2;
    m_mem_pipe_env->open_pipe()
        .then([&](pipe_wait_res, std::shared_ptr<ipipe_endpoint> pe) {
                  pe->write_async({});
                  pe->read_async();
                  pe2 = pe;
              });
    sync_event->wait();
}

TEST_F(ServiceConnector, ThrowsExceptionOnAttemptToStartTwice)
{
    auto sut = create_sut();
    sut->start();

    ASSERT_ANY_THROW(sut->start());
}
