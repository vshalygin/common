#include <rpc-lib/internal/connection/connection.h>
#include <rpc-lib/pipe/memory-pipe/mem-pipe-endpoint.h>
#include <rpc-lib/pipe/memory-pipe/mem-pipe-env.h>
#include <rpc-lib/internal/service/service.h>
#include <rpc-lib/internal/transfer-message/transfer-message.h>
#include <rpc-lib/closure-guard/closure-guard.h>

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/synchronization/event.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#pragma warning(pop)

using namespace vshalygin::rpc;
using namespace vshalygin::rpc::internal;
using namespace vshalygin::cl;
using namespace testing;

namespace {
    class DataHolderService
        : public proto::DataHolderService
    {
    public:
        void Set(::google::protobuf::RpcController *,
                 const ::proto::data_message *request,
                 ::proto::null_message *,
                 ::google::protobuf::Closure *done) override
        {
            closure_guard g(done);
            m_value = request->val();
        }

        void Get(::google::protobuf::RpcController *,
                 const ::proto::null_message *,
                 ::proto::data_message *response,
                 ::google::protobuf::Closure *done) override
        {
            closure_guard g(done);
            response->set_val(m_value);
        }

        void set_value(int64_t val)
        {
            m_value = val;
        }

    private:
        int64_t m_value = 0;
    };

    buffer create_req_set_data_message(int64_t val, uint64_t number)
    {
        proto::data_message msg;
        msg.set_val(val);
        return create_transfer_msg_req(number, 0, &msg);
    }

    buffer create_req_get_data_message(uint64_t number)
    {
        proto::null_message msg;
        return create_transfer_msg_req(number, 1, &msg);
    }

    buffer create_res_get_data_message(int64_t val, uint64_t number)
    {
        proto::data_message msg;
        msg.set_val(val);
        return create_transfer_msg_res(number, response_result::ok, &msg);
    }

    constexpr auto s_heartbeat_operation_timeout = std::chrono::seconds(5);

    struct heartbeat_read_result
    {
        bool completed = false;
        pipe_op_res result = pipe_op_res::failed;
        buffer message;
    };

    heartbeat_read_result read_heartbeat_message(
        const std::shared_ptr<ipipe_endpoint> &endpoint,
        std::chrono::milliseconds timeout)
    {
        auto future = endpoint->read_async();
        if(!future.wait_for(timeout)) {
            return {};
        }

        heartbeat_read_result result;
        result.completed = true;
        future.get().apply([&result](pipe_op_res r, buffer &&message) {
            result.result = r;
            result.message = std::move(message);
        });

        return result;
    }

    heartbeat_read_result read_heartbeat_message(
        const std::shared_ptr<ipipe_endpoint> &endpoint,
        transfer_msg_type expected_type,
        std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        do {
            const auto now = std::chrono::steady_clock::now();
            auto result = read_heartbeat_message(
                endpoint,
                std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now));
            if(!result.completed || result.result != pipe_op_res::success) {
                return result;
            }

            if(get_transfer_msg_type(result.message) == expected_type) {
                return result;
            }
        } while(std::chrono::steady_clock::now() < deadline);

        return {};
    }

    bool write_heartbeat_message(const std::shared_ptr<ipipe_endpoint> &endpoint,
                                 buffer &&message,
                                 std::chrono::milliseconds timeout)
    {
        auto future = endpoint->write_async(std::move(message));
        if(!future.wait_for(timeout)) {
            return false;
        }

        auto result = pipe_op_res::failed;
        future.get().apply([&result](pipe_op_res r) { result = r; });
        return result == pipe_op_res::success;
    }

    bool wait_until_connection_is_inactive(connection &value,
                                           std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        do {
            if(!value.is_active()) {
                return true;
            }

            std::this_thread::yield();
        } while(std::chrono::steady_clock::now() < deadline);

        return !value.is_active();
    }

    std::unique_ptr<connection> create_heartbeat_connection(
        const std::shared_ptr<thread_pool> &thread_pool,
        const std::shared_ptr<ipipe_endpoint> &endpoint,
        const std::shared_ptr<iservice> &service,
        std::chrono::milliseconds check_period,
        std::chrono::milliseconds ping_timeout)
    {
        return std::make_unique<connection>(thread_pool,
                                            endpoint,
                                            service,
                                            std::chrono::seconds(10),
                                            std::chrono::seconds(10),
                                            check_period,
                                            ping_timeout);
    }
}

class Connection
    : public Test
{
protected:
    void SetUp() override
    {
        m_data_holder_service = std::make_shared<DataHolderService>();
        m_thread_pool = std::make_shared<thread_pool>(2);

        mem_pipe_env env(m_thread_pool);
        auto m_pipe_env = mem_pipe_env(m_thread_pool);
        auto f1 = m_pipe_env.create_pipe()
            .then([&](pipe_wait_res, std::shared_ptr<ipipe_endpoint> p) {
            m_pipe_endpoint = std::move(p);
        });
        auto f2 = m_pipe_env.open_pipe()
            .then([&](pipe_wait_res, std::shared_ptr<ipipe_endpoint> p) {
            m_other_pipe_enpoint = std::move(p);
        });

        f1.get(); f2.get();
        m_service = std::make_shared<service<DataHolderService>>(m_data_holder_service,
                                                                 m_thread_pool,
                                                                 0);
    }

    void TearDown() override
    {
        m_service.reset();
        m_other_pipe_enpoint.reset();
        m_pipe_endpoint.reset();
        m_data_holder_service.reset();
        m_thread_pool->stop();
    }

    std::unique_ptr<connection> create_sut(const std::chrono::milliseconds &req_timeout,
                                           const std::chrono::milliseconds &res_timeout)
    {
        return std::make_unique<connection>(m_thread_pool,
                                            m_pipe_endpoint,
                                            m_service,
                                            req_timeout,
                                            res_timeout,
                                            std::chrono::seconds(10),
                                            std::chrono::seconds(10));
    }

    std::unique_ptr<connection> create_sut()
    {
        return create_sut(std::chrono::seconds(10), std::chrono::seconds(10));
    }

protected:
    std::shared_ptr<thread_pool> m_thread_pool;

    std::shared_ptr<DataHolderService> m_data_holder_service;

    std::shared_ptr<ipipe_endpoint> m_pipe_endpoint;
    std::shared_ptr<ipipe_endpoint> m_other_pipe_enpoint;
    std::shared_ptr<iservice> m_service;
};

TEST_F(Connection, IsNotActiveAfterConstruction)
{
    auto sut = create_sut();

    ASSERT_FALSE(sut->is_active());
}

TEST_F(Connection, IsActiveAfterStart)
{
    auto sut = create_sut();
    sut->start();

    ASSERT_TRUE(sut->is_active());
}

TEST_F(Connection, IsNotActiveAfterPipeInvalidation)
{
    auto sut = create_sut();
    sut->start();
    m_other_pipe_enpoint->invalidate();

    ASSERT_FALSE(sut->is_active());
}


TEST_F(Connection, SetStopCallback)
{
    auto sync_event = std::make_shared<event>();
    MockFunction<void()> stop_callback;
    EXPECT_CALL(stop_callback, Call)
        .Times(1)
        .WillOnce([sync_event]() { sync_event->set(); });

    auto sut = create_sut();
    sut->start();
    sut->set_stop_callback(thread_pool_task(m_thread_pool.get(), stop_callback.AsStdFunction()));
    sut->deactivate();

    sync_event->wait();
    Mock::VerifyAndClearExpectations(&stop_callback);
}

TEST_F(Connection, SetStopCallback2)
{
    auto sync_event = std::make_shared<event>();
    MockFunction<void()> stop_callback;
    EXPECT_CALL(stop_callback, Call)
        .Times(1)
        .WillOnce([sync_event]() { sync_event->set(); });

    auto sut = create_sut();
    sut->start();
    sut->set_stop_callback(thread_pool_task(m_thread_pool.get(), stop_callback.AsStdFunction()));
    m_other_pipe_enpoint->invalidate();

    sync_event->wait();
    Mock::VerifyAndClearExpectations(&stop_callback);
}

TEST_F(Connection, SetStopCallback3)
{
    auto sync_event = std::make_shared<event>();
    MockFunction<void()> stop_callback;
    EXPECT_CALL(stop_callback, Call)
        .Times(1)
        .WillOnce([sync_event]() { sync_event->set(); });

    auto sut = create_sut();
    sut->start();
    sut->set_stop_callback(thread_pool_task(m_thread_pool.get(), stop_callback.AsStdFunction()));
    sut.reset();

    sync_event->wait();
    Mock::VerifyAndClearExpectations(&stop_callback);
}

TEST_F(Connection, ReturnFailOnAttemptToMakeRequestOnInactiveTransport)
{
    auto sut = create_sut();

    auto f = sut->request_async(create_req_get_data_message(1));

    f.get().apply([](request_result r, buffer &&) { ASSERT_EQ(r, request_result::failed); });
}

TEST_F(Connection, ReturnFailOnAttemptToMakeRequestOnInactiveTransport2)
{
    auto sut = create_sut();
    sut->start();
    m_other_pipe_enpoint->invalidate();

    auto f = sut->request_async(create_req_get_data_message(1));

    f.get().apply([](request_result r, buffer &&) { ASSERT_EQ(r, request_result::failed); });
}

TEST_F(Connection, HasZeroPendingRequestsAfterCreation)
{
    auto sut = create_sut();
    sut->start();

    EXPECT_EQ(0, sut->get_active_timers_count());
    EXPECT_EQ(0, sut->get_pending_requests_count());
}

TEST_F(Connection, SendsARequest)
{
    auto msg = create_req_get_data_message(1);
    auto sut = create_sut();
    sut->start();

    sut->request_async(msg.copy());

    m_other_pipe_enpoint->read_async()
        .get().apply([&](pipe_op_res, buffer &&b) { ASSERT_TRUE(b == msg); });
    EXPECT_EQ(1, sut->get_active_timers_count());
    EXPECT_EQ(1, sut->get_pending_requests_count());
}

TEST_F(Connection, CancelActiveRequestOnDeactivation)
{
    auto msg = create_req_get_data_message(1);
    auto sut = create_sut();
    sut->start();

    auto f = sut->request_async(msg.copy());
    m_other_pipe_enpoint->read_async().wait();
    sut->deactivate();
    
    f.get().apply([](request_result r, buffer &&) { ASSERT_EQ(r, request_result::canceled); });
    while(sut->get_active_timers_count()) {}
    EXPECT_EQ(0, sut->get_pending_requests_count());
}

TEST_F(Connection, CancelActiveRequestOnDestroy)
{
    auto msg = create_req_get_data_message(1);
    auto sut = create_sut();
    sut->start();

    auto f = sut->request_async(msg.copy());
    m_other_pipe_enpoint->read_async().wait();
    sut.reset();

    f.get().apply([](request_result r, buffer &&) { ASSERT_EQ(r, request_result::canceled); });
}

TEST_F(Connection, MakesSuccessRequest)
{
    auto req_msg = create_req_get_data_message(1);
    auto res_msg = create_res_get_data_message(24, 1);
    auto sut = create_sut();
    sut->start();

    auto f = sut->request_async(req_msg.copy());
    m_other_pipe_enpoint->write_async(res_msg.copy());

    f.get().apply([&](request_result r, buffer &&b) {
        ASSERT_EQ(r, request_result::ok);
        ASSERT_TRUE(b == res_msg);
    });

    while(sut->get_active_timers_count()) {}
    EXPECT_EQ(0, sut->get_pending_requests_count());
}

TEST_F(Connection, IgnoresResponseWithWrongNumber)
{
    auto req_msg = create_req_get_data_message(1);
    auto res_msg = create_res_get_data_message(24, 2);
    auto sut = create_sut();
    sut->start();

    sut->request_async(req_msg.copy());
    m_other_pipe_enpoint->write_async(res_msg.copy());

    EXPECT_EQ(1, sut->get_active_timers_count());
    EXPECT_EQ(1, sut->get_pending_requests_count());
}

TEST_F(Connection, WriteOperationCanceled)
{
    auto sync_event = std::make_shared<event>();
    for(unsigned i = 0; i < m_thread_pool->get_num(); ++i) {
        m_thread_pool->post([sync_event]() { sync_event->wait(); });
    }
    auto req_msg = create_req_get_data_message(1);
    auto sut = create_sut();
    sut->start();

    auto f = sut->request_async(req_msg.copy());
    m_pipe_endpoint->invalidate();

    sync_event->set();
    f.get().apply([&](request_result r, buffer &&) { EXPECT_EQ(r, request_result::send_canceled); });
    while(sut->get_active_timers_count()) {}
    EXPECT_EQ(0, sut->get_pending_requests_count());
}

TEST_F(Connection, WriteOperationTimeout)
{
    auto sync_event = std::make_shared<event>();
    for(unsigned i = 0; i < m_thread_pool->get_num(); ++i) {
        m_thread_pool->post([sync_event]() { sync_event->wait(); });
    }
    auto req_msg = create_req_get_data_message(1);
    auto sut = create_sut(std::chrono::milliseconds(0), std::chrono::milliseconds(10000));
    sut->start();

    auto f = sut->request_async(req_msg.copy());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event->set();
    f.get().apply([&](request_result r, buffer &&) { EXPECT_EQ(r, request_result::send_timeout); });
    while(sut->get_active_timers_count()) {}
    EXPECT_EQ(0, sut->get_pending_requests_count());
}

TEST_F(Connection, RequestTimeout)
{
    auto sync_event = std::make_shared<event>();
    for(unsigned i = 0; i < m_thread_pool->get_num(); ++i) {
        m_thread_pool->post([sync_event]() { sync_event->wait(); });
    }
    auto req_msg = create_req_get_data_message(1);
    auto sut = create_sut(std::chrono::milliseconds(10000), std::chrono::milliseconds(0));
    sut->start();

    auto f = sut->request_async(req_msg.copy());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event->set();
    f.get().apply([&](request_result r, buffer &&) { EXPECT_EQ(r, request_result::timeout); });
    while(sut->get_active_timers_count()) {}
    EXPECT_EQ(0, sut->get_pending_requests_count());
}

TEST_F(Connection, ProcessRequestAsServer)
{
    m_data_holder_service->set_value(34);
    auto req_msg = create_req_get_data_message(1);
    auto expected_res_msg = create_res_get_data_message(34, 1);
    auto sut = create_sut();
    sut->start();

    m_other_pipe_enpoint->write_async(req_msg.copy());
    auto f = m_other_pipe_enpoint->read_async();

    f.get().apply([&](pipe_op_res p, buffer &b) {
        EXPECT_EQ(p, pipe_op_res::success);
        EXPECT_TRUE(expected_res_msg == b);
    });
}

TEST_F(Connection, CancelsActiveRequestOnDeactivate)
{
    auto req_msg = create_req_get_data_message(1);
    auto sut = create_sut();
    sut->start();

    auto f = sut->request_async(req_msg.copy());
    m_other_pipe_enpoint->read_async().get();
    sut->deactivate();

    f.get().apply([&](request_result r, buffer &&) {
        ASSERT_EQ(r, request_result::canceled);
    });

    while(sut->get_active_timers_count()) {}
    EXPECT_EQ(0, sut->get_pending_requests_count());
}

TEST_F(Connection, CancelsActiveRequestOnConnectionLost)
{
    auto req_msg = create_req_get_data_message(1);
    auto sut = create_sut();
    sut->start();

    auto f = sut->request_async(req_msg.copy());
    m_other_pipe_enpoint->read_async().wait();
    m_other_pipe_enpoint->invalidate();

    f.get().apply([&](request_result r, buffer &&) {
        ASSERT_EQ(r, request_result::failed);
    });

    while(sut->get_active_timers_count()) {}
    EXPECT_EQ(0, sut->get_pending_requests_count());
}

TEST_F(Connection, SendsHeartbeatPingAfterInactivity)
{
    auto sut = create_heartbeat_connection(m_thread_pool,
                                           m_pipe_endpoint,
                                           m_service,
                                           std::chrono::milliseconds(1),
                                           std::chrono::hours(1));
    sut->start();

    auto read = read_heartbeat_message(m_other_pipe_enpoint, s_heartbeat_operation_timeout);

    ASSERT_TRUE(read.completed);
    ASSERT_EQ(read.result, pipe_op_res::success);
    EXPECT_EQ(get_transfer_msg_type(read.message), transfer_msg_type::ping);
}

TEST_F(Connection, SendsRepeatedHeartbeatPingsWhileWaitingForActivity)
{
    auto sut = create_heartbeat_connection(m_thread_pool,
                                           m_pipe_endpoint,
                                           m_service,
                                           std::chrono::milliseconds(1),
                                           std::chrono::hours(1));
    sut->start();

    auto first = read_heartbeat_message(m_other_pipe_enpoint, s_heartbeat_operation_timeout);
    ASSERT_TRUE(first.completed);
    ASSERT_EQ(first.result, pipe_op_res::success);
    EXPECT_EQ(get_transfer_msg_type(first.message), transfer_msg_type::ping);

    auto second = read_heartbeat_message(m_other_pipe_enpoint, s_heartbeat_operation_timeout);
    ASSERT_TRUE(second.completed);
    ASSERT_EQ(second.result, pipe_op_res::success);
    EXPECT_EQ(get_transfer_msg_type(second.message), transfer_msg_type::ping);
}

TEST_F(Connection, RepliesToHeartbeatPingWithPong)
{
    auto sut = create_heartbeat_connection(m_thread_pool,
                                           m_pipe_endpoint,
                                           m_service,
                                           std::chrono::hours(1),
                                           std::chrono::hours(1));
    sut->start();

    ASSERT_TRUE(write_heartbeat_message(m_other_pipe_enpoint,
                                        create_transfer_msg_ping(),
                                        s_heartbeat_operation_timeout));
    auto read = read_heartbeat_message(m_other_pipe_enpoint, s_heartbeat_operation_timeout);

    ASSERT_TRUE(read.completed);
    ASSERT_EQ(read.result, pipe_op_res::success);
    EXPECT_EQ(get_transfer_msg_type(read.message), transfer_msg_type::pong);
}

TEST_F(Connection, PongIsProcessedWhileWaitingForHeartbeatResponse)
{
    auto sut = create_heartbeat_connection(m_thread_pool,
                                           m_pipe_endpoint,
                                           m_service,
                                           std::chrono::milliseconds(1),
                                           std::chrono::hours(1));
    sut->start();

    auto read = read_heartbeat_message(m_other_pipe_enpoint, s_heartbeat_operation_timeout);
    ASSERT_TRUE(read.completed);
    ASSERT_EQ(read.result, pipe_op_res::success);
    ASSERT_EQ(get_transfer_msg_type(read.message), transfer_msg_type::ping);

    ASSERT_TRUE(write_heartbeat_message(m_other_pipe_enpoint,
                                        create_transfer_msg_pong(),
                                        s_heartbeat_operation_timeout));
    ASSERT_TRUE(write_heartbeat_message(m_other_pipe_enpoint,
                                        create_transfer_msg_ping(),
                                        s_heartbeat_operation_timeout));

    auto pong = read_heartbeat_message(m_other_pipe_enpoint,
                                       transfer_msg_type::pong,
                                       s_heartbeat_operation_timeout);

    ASSERT_TRUE(pong.completed);
    ASSERT_EQ(pong.result, pipe_op_res::success);
    EXPECT_TRUE(sut->is_active());
    EXPECT_TRUE(m_other_pipe_enpoint->is_connected());
}

TEST_F(Connection, DeactivatesIfHeartbeatResponseDoesNotArrive)
{
    auto sut = create_heartbeat_connection(m_thread_pool,
                                           m_pipe_endpoint,
                                           m_service,
                                           std::chrono::milliseconds(0),
                                           std::chrono::milliseconds(0));
    sut->start();

    ASSERT_TRUE(wait_until_connection_is_inactive(*sut, s_heartbeat_operation_timeout));

    EXPECT_FALSE(sut->is_active());
    EXPECT_FALSE(m_other_pipe_enpoint->is_connected());
}

TEST_F(Connection, SecondStartDoesNotThrowAndKeepsConnectionActive)
{
    auto sut = create_heartbeat_connection(m_thread_pool,
                                           m_pipe_endpoint,
                                           m_service,
                                           std::chrono::hours(1),
                                           std::chrono::hours(1));
    sut->start();

    EXPECT_NO_THROW(sut->start());
    EXPECT_TRUE(sut->is_active());
}

TEST_F(Connection, StartAfterDeactivationDoesNothing)
{
    auto sut = create_heartbeat_connection(m_thread_pool,
                                           m_pipe_endpoint,
                                           m_service,
                                           std::chrono::milliseconds(10),
                                           std::chrono::milliseconds(80));

    sut->deactivate();
    EXPECT_NO_THROW(sut->start());

    EXPECT_FALSE(sut->is_active());
    EXPECT_FALSE(m_other_pipe_enpoint->is_connected());
}

TEST_F(Connection, DoesNotRestartHeartbeatAfterDeactivation)
{
    auto sut = create_heartbeat_connection(m_thread_pool,
                                           m_pipe_endpoint,
                                           m_service,
                                           std::chrono::milliseconds(10),
                                           std::chrono::milliseconds(80));
    sut->start();
    sut->deactivate();

    EXPECT_NO_THROW(sut->start());

    EXPECT_FALSE(sut->is_active());
    EXPECT_FALSE(m_other_pipe_enpoint->is_connected());
}
