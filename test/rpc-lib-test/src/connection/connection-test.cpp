#include <rpc-lib/connection/connection.h>
#include <rpc-lib/pipe/memory-pipe/mem-pipe-endpoint.h>
#include <rpc-lib/pipe/memory-pipe/mem-pipe-env.h>
#include <rpc-lib/service/service.h>
#include <rpc-lib/transfer-message/transfer-message.h>
#include <rpc-lib/closure-guard/closure-guard.h>

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/synchronization/event/event.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#pragma warning(pop)

using namespace vshalygin::rpc;
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
                                            res_timeout);
    }

    std::unique_ptr<connection> create_sut()
    {
        return create_sut(std::chrono::milliseconds(10000), std::chrono::milliseconds(10000));
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
    event sync_event;
    MockFunction<void()> stop_callback;
    EXPECT_CALL(stop_callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    auto sut = create_sut();
    sut->start();
    sut->set_stop_callback(stop_callback.AsStdFunction());
    sut->deactivate();

    sync_event.wait();
    Mock::VerifyAndClearExpectations(&stop_callback);
}

TEST_F(Connection, SetStopCallback2)
{
    event sync_event;
    MockFunction<void()> stop_callback;
    EXPECT_CALL(stop_callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    auto sut = create_sut();
    sut->start();
    sut->set_stop_callback(stop_callback.AsStdFunction());
    m_other_pipe_enpoint->invalidate();

    sync_event.wait();
    Mock::VerifyAndClearExpectations(&stop_callback);
}

TEST_F(Connection, SetStopCallback3)
{
    event sync_event;
    MockFunction<void()> stop_callback;
    EXPECT_CALL(stop_callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    auto sut = create_sut();
    sut->start();
    sut->set_stop_callback(stop_callback.AsStdFunction());
    sut.reset();

    sync_event.wait();
    Mock::VerifyAndClearExpectations(&stop_callback);
}

TEST_F(Connection, ThrowsExceptionOnAttemptToMakeRequestOnInactiveTransport)
{
    auto sut = create_sut();
    
    ASSERT_ANY_THROW(sut->request_async(create_req_get_data_message(1)));
}

TEST_F(Connection, ThrowsExceptionOnAttemptToMakeRequestOnInactiveTransport2)
{
    auto sut = create_sut();
    sut->start();
    m_other_pipe_enpoint->invalidate();

    ASSERT_ANY_THROW(sut->request_async(create_req_get_data_message(1)));
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

TEST_F(Connection, WriteOperationFails)
{
    event sync_event;
    for(unsigned i = 0; i < m_thread_pool->get_num(); ++i) {
        m_thread_pool->post([&]() { sync_event.wait(); });
    }
    auto req_msg = create_req_get_data_message(1);
    auto sut = create_sut();
    sut->start();

    auto f = sut->request_async(req_msg.copy());
    m_pipe_endpoint->invalidate();

    sync_event.set();
    f.get().apply([&](request_result r, buffer &&) { EXPECT_EQ(r, request_result::send_unknown_error); });
    while(sut->get_active_timers_count()) {}
    EXPECT_EQ(0, sut->get_pending_requests_count());
}

TEST_F(Connection, WriteOperationTimeout)
{
    event sync_event;
    for(unsigned i = 0; i < m_thread_pool->get_num(); ++i) {
        m_thread_pool->post([&]() { sync_event.wait(); });
    }
    auto req_msg = create_req_get_data_message(1);
    auto sut = create_sut(std::chrono::milliseconds(0), std::chrono::milliseconds(10000));
    sut->start();

    auto f = sut->request_async(req_msg.copy());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event.set();
    f.get().apply([&](request_result r, buffer &&) { EXPECT_EQ(r, request_result::send_timeout_error); });
    while(sut->get_active_timers_count()) {}
    EXPECT_EQ(0, sut->get_pending_requests_count());
}

TEST_F(Connection, RequestTimeout)
{
    event sync_event;
    for(unsigned i = 0; i < m_thread_pool->get_num(); ++i) {
        m_thread_pool->post([&]() { sync_event.wait(); });
    }
    auto req_msg = create_req_get_data_message(1);
    auto sut = create_sut(std::chrono::milliseconds(10000), std::chrono::milliseconds(0));
    sut->start();

    auto f = sut->request_async(req_msg.copy());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event.set();
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
