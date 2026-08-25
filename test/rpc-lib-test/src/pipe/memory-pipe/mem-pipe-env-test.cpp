#include <rpc-lib/pipe/memory-pipe/mem-pipe-env.h>
#include <rpc-lib/pipe/ipipe-endpoint.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::rpc;
using namespace vshalygin::cl;
using namespace testing;

class MemPipeEnv
    : public Test
{
protected:
    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(2);
        m_sut = std::make_unique<mem_pipe_env>(m_thread_pool.get());
    }

    void TearDown() override
    {
        m_sut.reset();
        m_thread_pool->stop();
    }

protected:
    std::shared_ptr<thread_pool> m_thread_pool;
    std::unique_ptr<mem_pipe_env> m_sut;
};

TEST_F(MemPipeEnv, CreatePipeFuture)
{
    auto f = m_sut->create_pipe(0); (void)f;

    EXPECT_EQ(m_sut->get_pending_client_endpoints_count(), 0u);
    EXPECT_EQ(m_sut->get_pending_server_endpoints_count(), 1u);
}

TEST_F(MemPipeEnv, OpenPipeFuture)
{
    auto f = m_sut->open_pipe(0); (void)f;

    EXPECT_EQ(m_sut->get_pending_client_endpoints_count(), 1u);
    EXPECT_EQ(m_sut->get_pending_server_endpoints_count(), 0u);
}

TEST_F(MemPipeEnv, StopPendingServerEndpoint)
{
    auto f = m_sut->create_pipe(0);

    m_sut->cancel_all_pending_server_endpoints();
    
    f.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint>) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
    });
    EXPECT_EQ(m_sut->get_pending_client_endpoints_count(), 0u);
    EXPECT_EQ(m_sut->get_pending_server_endpoints_count(), 0u);
}

TEST_F(MemPipeEnv, StopPendingClientEndpoint)
{
    auto f = m_sut->open_pipe(0);

    m_sut->cancel_all_pending_client_endpoints();

    f.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint>) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
    });
    EXPECT_EQ(m_sut->get_pending_client_endpoints_count(), 0u);
    EXPECT_EQ(m_sut->get_pending_server_endpoints_count(), 0u);
}

TEST_F(MemPipeEnv, StopPendingServerEndpointOnDestruction)
{
    auto f = m_sut->create_pipe(0);

    m_sut.reset();

    f.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint>) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
    });
}

TEST_F(MemPipeEnv, StopPendingClientEndpointOnDestruction)
{
    auto f = m_sut->open_pipe(0);

    m_sut.reset();

    f.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint>) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
    });
}

TEST_F(MemPipeEnv, CreateConnectedEndpoints)
{
    auto f1 = m_sut->open_pipe(0);
    auto f2 = m_sut->create_pipe(0);

    std::shared_ptr<ipipe_endpoint> server_endpoint;
    std::shared_ptr<ipipe_endpoint> client_endpoint;
    f1.get().apply([&](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> p) {
        ASSERT_EQ(r, pipe_wait_res::success);
        ASSERT_TRUE(p);
        
        client_endpoint = std::move(p);
    });
    f2.get().apply([&](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> p) {
        ASSERT_EQ(r, pipe_wait_res::success);
        ASSERT_TRUE(p);

        server_endpoint = std::move(p);
    });

    buffer buf(2); buf[0] = std::byte(1);
    server_endpoint->write_async(buf.copy()).get();
    client_endpoint->read_async()
        .get()
        .apply([&](pipe_op_res r, buffer &&b) {
            EXPECT_EQ(r, pipe_op_res::success);
            EXPECT_TRUE(b == buf);
        });
    EXPECT_EQ(m_sut->get_pending_client_endpoints_count(), 0u);
    EXPECT_EQ(m_sut->get_pending_server_endpoints_count(), 0u);
}

TEST_F(MemPipeEnv, CancelWaitingByTimer)
{
    auto f = m_sut->create_pipe(0, std::chrono::milliseconds(1));

    f.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint>) {
        EXPECT_EQ(r, pipe_wait_res::timeout);
    });

    EXPECT_EQ(m_sut->get_pending_client_endpoints_count(), 0u);
    EXPECT_EQ(m_sut->get_pending_server_endpoints_count(), 0u);
}

TEST_F(MemPipeEnv, CancelsOnlyPendingServerEndpointsWithSpecifiedClientId)
{
    constexpr auto canceled_client_id = 101u;
    constexpr auto remaining_client_id = 202u;
    auto first_canceled = m_sut->create_pipe(canceled_client_id);
    auto second_canceled = m_sut->create_pipe(canceled_client_id);
    auto remaining = m_sut->create_pipe(remaining_client_id);

    m_sut->cancel_pending_server_endpoints(canceled_client_id);

    EXPECT_EQ(m_sut->get_pending_server_endpoints_count(), 1u);
    first_canceled.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> endpoint) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
        EXPECT_FALSE(endpoint);
    });
    second_canceled.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> endpoint) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
        EXPECT_FALSE(endpoint);
    });

    m_sut->cancel_pending_server_endpoints(remaining_client_id);
    remaining.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> endpoint) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
        EXPECT_FALSE(endpoint);
    });
    EXPECT_EQ(m_sut->get_pending_server_endpoints_count(), 0u);
}

TEST_F(MemPipeEnv, CancelsOnlyPendingClientEndpointsWithSpecifiedClientId)
{
    constexpr auto canceled_client_id = 101u;
    constexpr auto remaining_client_id = 202u;
    auto first_canceled = m_sut->open_pipe(canceled_client_id);
    auto second_canceled = m_sut->open_pipe(canceled_client_id);
    auto remaining = m_sut->open_pipe(remaining_client_id);

    m_sut->cancel_pending_client_endpoints(canceled_client_id);

    EXPECT_EQ(m_sut->get_pending_client_endpoints_count(), 1u);
    first_canceled.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> endpoint) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
        EXPECT_FALSE(endpoint);
    });
    second_canceled.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> endpoint) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
        EXPECT_FALSE(endpoint);
    });

    m_sut->cancel_pending_client_endpoints(remaining_client_id);
    remaining.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> endpoint) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
        EXPECT_FALSE(endpoint);
    });
    EXPECT_EQ(m_sut->get_pending_client_endpoints_count(), 0u);
}

TEST_F(MemPipeEnv, CancelsAllPendingServerEndpointsWithDifferentClientIds)
{
    auto first = m_sut->create_pipe(101);
    auto second = m_sut->create_pipe(202);
    auto third = m_sut->create_pipe(303);

    m_sut->cancel_all_pending_server_endpoints();

    first.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> endpoint) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
        EXPECT_FALSE(endpoint);
    });
    second.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> endpoint) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
        EXPECT_FALSE(endpoint);
    });
    third.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> endpoint) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
        EXPECT_FALSE(endpoint);
    });
    EXPECT_EQ(m_sut->get_pending_server_endpoints_count(), 0u);
}

TEST_F(MemPipeEnv, CancelsAllPendingClientEndpointsWithDifferentClientIds)
{
    auto first = m_sut->open_pipe(101);
    auto second = m_sut->open_pipe(202);
    auto third = m_sut->open_pipe(303);

    m_sut->cancel_all_pending_client_endpoints();

    first.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> endpoint) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
        EXPECT_FALSE(endpoint);
    });
    second.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> endpoint) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
        EXPECT_FALSE(endpoint);
    });
    third.get().apply([](pipe_wait_res r, std::shared_ptr<ipipe_endpoint> endpoint) {
        EXPECT_EQ(r, pipe_wait_res::canceled);
        EXPECT_FALSE(endpoint);
    });
    EXPECT_EQ(m_sut->get_pending_client_endpoints_count(), 0u);
}
