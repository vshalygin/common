#include <rpc-lib/connection/connection.h>
#include <rpc-lib/transfer-message/transfer-message.h>

#include "mocks/multiple-timer-mock.h"

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#pragma warning(pop)

#include <common-lib/thread-pool/thread-pool.h>
#include <common-lib/syncronization/event/event.h>

#include <list>

using namespace vsh::cl;
using namespace vsh::rpc;
using namespace testing;

namespace {
    class transport_mock
        : public itransport
    {
    public:
        transport_mock()
        {
            was_stopped = false;
            was_started = false;
        }

        MOCK_METHOD(void, send_async, (buffer &&message,
                                       std::function<void()> &&error_handler), (const, override));

        void recv_async(std::function<void(buffer &&)> &&handler) const override
        {
            m_recv_handler = std::move(handler);
            ++m_recv_async_called;
        }

        void start() override
        {
            was_started = true;
            m_stopped = false;
            if(m_start_handler) {
                m_start_handler();
            }
        }

        void stop() override
        {
            was_stopped = true;
            m_stopped = true;
            if(m_stop_handler) {
                m_stop_handler();
            }
        }

        bool is_stopped() const override
        {
            return m_stopped;
        }

        void set_start_callback(std::function<void()> &&handler) override
        {
            m_start_handler = std::move(handler);
        }

        void set_stop_callback(std::function<void()> &&handler) override
        {
            m_stop_handler = std::move(handler);
        }

        void emit_disconnect_event()
        {
            ASSERT_TRUE(m_stop_handler) << "disconnect handler is not set";
            m_stop_handler();
        }

        void emit_recv_event(buffer &&message)
        {
            ASSERT_TRUE(m_recv_handler) << "recv_async was never called";
            m_recv_handler(std::move(message));
        }

        int get_recv_async_called() const
        {
            return m_recv_async_called;
        }

        inline static bool was_stopped = false;
        inline static bool was_started = false;

    private:
        mutable std::function<void(buffer &&)> m_recv_handler;
        std::function<void()> m_start_handler;
        std::function<void()> m_stop_handler;
        mutable int m_recv_async_called = 0;
        bool m_stopped = true;
    };

    using transport_nice_mock = ::NiceMock<transport_mock>;
}

class Connection
    : public Test
{
protected:
    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(2);
        m_transport = std::make_unique<transport_nice_mock>();
        m_multiple_timer = std::make_unique<multiple_timer_nice_mock>();

        m_transport_ptr = m_transport.get();
        m_multiple_timer_ptr = m_multiple_timer.get();

        m_request_message.set_some_data(34);
        m_response_message.set_some_data(43);
    }

    void TearDown() override
    {
        m_thread_pool->stop();
    }

    std::unique_ptr<iconnection> create_sut()
    {
        auto ans = std::make_unique<connection>(std::move(m_multiple_timer),
                                                m_thread_pool);
        ans->set_and_start_transport(std::move(m_transport));
        return ans;
    }

    std::unique_ptr<iconnection> create_sut_without_transport()
    {
        auto ans = std::make_unique<connection>(std::move(m_multiple_timer),
                                                m_thread_pool);
        return ans;
    }

protected:
    std::shared_ptr<thread_pool> m_thread_pool;
    transport_nice_mock *m_transport_ptr;
    multiple_timer_nice_mock  *m_multiple_timer_ptr;

    proto::some_message m_request_message;
    proto::some_message m_response_message;

    std::unique_ptr<transport_nice_mock> m_transport;
private:
    std::unique_ptr<multiple_timer_nice_mock> m_multiple_timer;
};

TEST_F(Connection, LaunchesTimerOnRequestAsyncOperation)
{
    EXPECT_CALL(*m_multiple_timer_ptr, start(_, std::chrono::microseconds(10000000)))
        .Times(1);
    auto req_message = create_transfer_msg_req(3, 7, &m_request_message);
    auto sut = create_sut();

    sut->request_async(std::move(req_message), {});
}

TEST_F(Connection, LaunchesRequestAsyncOperation)
{
    auto req_message = create_transfer_msg_req(3, 7, &m_request_message);
    EXPECT_CALL(*m_transport_ptr, send_async)
        .Times(1)
        .WillOnce([req_message = req_message.copy()](auto buf, auto) { EXPECT_EQ(req_message, buf); });

    auto sut = create_sut();
    sut->request_async(std::move(req_message), {});
}

TEST_F(Connection, ProcessesZeroRequestAfterCreation)
{
    auto sut = create_sut();

    ASSERT_EQ(sut->get_active_requests_count(), 0);
}

TEST_F(Connection, AddsProcessingRequestAfterReqAsyncCalled)
{
    auto req_message = create_transfer_msg_req(3, 7, &m_request_message);
    auto sut = create_sut();
    sut->request_async(std::move(req_message), {});

    ASSERT_EQ(sut->get_active_requests_count(), 1);
}

TEST_F(Connection, DoNotCallEmptyCallbackOnRequestAsyncOperation)
{
    auto transfer_req_message = create_transfer_msg_req(3, 7, &m_request_message);
    auto transfer_res_message = create_transfer_msg_res(
        3, response_result::unknown_error, &m_response_message);

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), {});
    m_transport_ptr->emit_recv_event(std::move(transfer_res_message));
}

TEST_F(Connection, ClearsRequestAsyncOperationAfterItsCompleted)
{
    auto transfer_req_message = create_transfer_msg_req(3, 7, &m_request_message);
    auto transfer_res_message = create_transfer_msg_res(
        3, response_result::unknown_error, &m_response_message);

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), {});
    m_transport_ptr->emit_recv_event(std::move(transfer_res_message));

    ASSERT_EQ(sut->get_active_requests_count(), 0);
}

TEST_F(Connection, ProcessesResponseToRequestAsync)
{
    event sync_event;
    auto transfer_req_message = create_transfer_msg_req(3, 7, &m_request_message);
    auto transfer_res_message = create_transfer_msg_res(
        3, response_result::unknown_error, &m_response_message);
    MockFunction<void(request_result, buffer &&)> mock_function;
    EXPECT_CALL(mock_function, Call(request_result::ok, _))
        .Times(1)
        .WillOnce([transfer_res_msg = transfer_res_message.copy(), &sync_event](request_result, buffer &&message) {
                      EXPECT_EQ(transfer_res_msg, message);
                      sync_event.set();
                  });

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), mock_function.AsStdFunction());
    m_transport_ptr->emit_recv_event(std::move(transfer_res_message));
    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(Connection, CallsRequestCallbackOnlyOnCorrespondingResponse)
{
    event sync_event;
    const uint64_t num = 34;
    auto transfer_req_message = create_transfer_msg_req(num, 7, &m_request_message);
    auto transfer_res_message = create_transfer_msg_res(
        num+1, response_result::unknown_error, &m_response_message);
    MockFunction<void(request_result, buffer &&)> mock_function;
    EXPECT_CALL(mock_function, Call(request_result::ok, _))
        .Times(0);
    EXPECT_CALL(mock_function, Call(request_result::canceled, _))
        .Times(1)
        .WillOnce([&sync_event]() { sync_event.set(); });

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), mock_function.AsStdFunction());
    m_transport_ptr->emit_recv_event(std::move(transfer_res_message));

    sut.reset();
    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(Connection, CatchesExceptionsThrownByRequestCallback)
{
    const uint64_t num = 34;

    auto transfer_req_message = create_transfer_msg_req(num, 7, &m_request_message);
    auto transfer_res_message = create_transfer_msg_res(
        num, response_result::unknown_error, &m_response_message);
    MockFunction<void(request_result, buffer &&)> mock_function;
    EXPECT_CALL(mock_function, Call)
        .Times(1)
        .WillOnce(Throw(std::exception()));

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), mock_function.AsStdFunction());
    
    m_transport_ptr->emit_recv_event(std::move(transfer_res_message));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ASSERT_EQ(sut->get_active_requests_count(), 0);
}

TEST_F(Connection, CancelsTimerAfterRequestCallbackInvoked)
{
    event sync_event;
    const uint64_t timer_id = 546456;
    EXPECT_CALL(*m_multiple_timer_ptr, start)
        .Times(1)
        .WillOnce(Return(timer_id));
    EXPECT_CALL(*m_multiple_timer_ptr, cancel(timer_id))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    const uint64_t num = 34;
    auto transfer_req_message = create_transfer_msg_req(num, 7, &m_request_message);
    auto transfer_res_message = create_transfer_msg_res(
        num, response_result::unknown_error, &m_response_message);

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), {});

    m_transport_ptr->emit_recv_event(std::move(transfer_res_message));
    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(Connection, ProcessedRequestTimeout)
{
    auto pool = std::make_shared<thread_pool>(1);
    event sync_event;
    auto transfer_req_message = create_transfer_msg_req(34, 7, &m_request_message);
    MockFunction<void(request_result, buffer &&)> mock_function;
    EXPECT_CALL(mock_function, Call(request_result::timeout, _))
        .Times(1)
        .WillOnce([&sync_event](request_result, buffer &&buff) {
                      EXPECT_EQ(buff.size(), 0);
                      sync_event.set();
                  });
    EXPECT_CALL(*m_multiple_timer_ptr, start)
        .Times(1)
        .WillOnce([pool](auto &&callback, const auto &microseconds) {
                      pool->post(callback);
                      return 0;
                  });

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), mock_function.AsStdFunction());
    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(Connection, ClearsRequestAsyncOperationAfterItsCanceledByTimer)
{
    auto pool = std::make_shared<thread_pool>(1);
    event sync_event;
    auto transfer_req_message = create_transfer_msg_req(34, 7, &m_request_message);
    NiceMock<MockFunction<void(request_result, buffer &&)>> mock_function;
    ON_CALL(mock_function, Call)
        .WillByDefault([&sync_event]() { sync_event.set(); });
    ON_CALL(*m_multiple_timer_ptr, start)
        .WillByDefault([pool](auto &&callback, const auto &microseconds) {
                           pool->post(callback);
                           return 0;
                       });

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), mock_function.AsStdFunction());
    sync_event.wait_for(std::chrono::seconds(10));

    ASSERT_EQ(sut->get_active_requests_count(), 0);
}

TEST_F(Connection, ClearsRequestAsyncOperationAfterItsCanceledByTimerIfCallbackThrowsException)
{
    auto pool = std::make_shared<thread_pool>(1);
    event sync_event;
    auto transfer_req_message = create_transfer_msg_req(34, 7, &m_request_message);
    NiceMock<MockFunction<void(request_result, buffer &&)>> mock_function;
    ON_CALL(mock_function, Call)
        .WillByDefault(DoAll([&sync_event]() { sync_event.set(); }, Throw(std::exception()) ));
    ON_CALL(*m_multiple_timer_ptr, start)
        .WillByDefault([pool](auto &&callback, const auto &microseconds) {
                           pool->post(std::move(callback));
                           return 0;
                       });

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), mock_function.AsStdFunction());
    sync_event.wait_for(std::chrono::seconds(10));
    pool->stop();

    ASSERT_EQ(sut->get_active_requests_count(), 0);
}

TEST_F(Connection, ProcessedRequestSendError)
{
    auto pool = std::make_shared<thread_pool>(1);
    event sync_event;
    auto transfer_req_message = create_transfer_msg_req(34, 7, &m_request_message);
    MockFunction<void(request_result, buffer &&)> mock_function;
    EXPECT_CALL(mock_function, Call(request_result::send_error, _))
        .Times(1)
        .WillOnce([&sync_event](request_result, buffer &&buff) {
                       EXPECT_TRUE(buff.size() == 0);
                       sync_event.set();
                   });
    EXPECT_CALL(*m_transport_ptr, send_async)
        .Times(1)
        .WillOnce([&](auto, auto &&callback) { pool->post(std::move(callback)); });

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), mock_function.AsStdFunction());
    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(Connection, ClearsRequestAsyncOperationAfterItsCanceledBySendingError)
{
    auto pool = std::make_shared<thread_pool>(1);
    event sync_event;
    auto transfer_req_message = create_transfer_msg_req(34, 7, &m_request_message);
    NiceMock<MockFunction<void(request_result, buffer &&)>> mock_function;
    ON_CALL(mock_function, Call)
        .WillByDefault([&sync_event]() { sync_event.set(); });
    ON_CALL(*m_transport_ptr, send_async)
        .WillByDefault([&](auto, auto &&callback) { pool->post(std::move(callback)); });

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), mock_function.AsStdFunction());
    sync_event.wait_for(std::chrono::seconds(10));

    ASSERT_EQ(sut->get_active_requests_count(), 0);
}

TEST_F(Connection, ClearsRequestAsyncOperationAfterSendingErrorIfCallbackThrowsException)
{
    auto pool = std::make_shared<thread_pool>(1);
    event sync_event;
    auto transfer_req_message = create_transfer_msg_req(34, 7, &m_request_message);
    NiceMock<MockFunction<void(request_result, buffer &&)>> mock_function;
    ON_CALL(mock_function, Call)
        .WillByDefault(DoAll([&sync_event]() { sync_event.set(); }, Throw(std::exception())));
    ON_CALL(*m_transport_ptr, send_async)
        .WillByDefault([&](auto, auto &&callback) { pool->post(std::move(callback)); });

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), mock_function.AsStdFunction());
    sync_event.wait_for(std::chrono::seconds(10));
    pool->stop();

    ASSERT_EQ(sut->get_active_requests_count(), 0);
}

TEST_F(Connection, ClearsRequestAsyncOperationIfSendAsyncThrowsException)
{
    event sync_event;
    auto transfer_req_message = create_transfer_msg_req(34, 7, &m_request_message);
    ON_CALL(*m_transport_ptr, send_async)
        .WillByDefault(Throw(std::exception()));
    EXPECT_CALL(*m_multiple_timer_ptr, start)
        .Times(1)
        .WillOnce(Return(35));
    EXPECT_CALL(*m_multiple_timer_ptr, cancel(35))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    auto sut = create_sut();
    ASSERT_ANY_THROW(sut->request_async(std::move(transfer_req_message), {}));
    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
    ASSERT_EQ(sut->get_active_requests_count(), 0);
}

TEST_F(Connection, MakesRecvAsyncAfterRecvAsyncCallbackCalled)
{
    const auto num = 34;
    auto transfer_req_message = create_transfer_msg_req(num, 7, &m_request_message);
    auto transfer_res_message = create_transfer_msg_res(
        num, response_result::unknown_error, &m_response_message);
    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), {});
    ASSERT_EQ(m_transport_ptr->get_recv_async_called(), 1);

    m_transport_ptr->emit_recv_event(std::move(transfer_res_message));
    ASSERT_EQ(m_transport_ptr->get_recv_async_called(), 2);
}

TEST_F(Connection, DoesNothingOnRequestIfRequestHandlerIsNotSet)
{
    auto transfer_req_message = create_transfer_msg_req(34, 7, &m_request_message);

    auto sut = create_sut();
    m_transport_ptr->emit_recv_event(std::move(transfer_req_message));
}

TEST_F(Connection, SendsAnswerOnRequest)
{
    event sync_event;
    const auto num = 34;
    auto transfer_req_message = create_transfer_msg_req(num, 7, &m_request_message);
    auto transfer_res_message = create_transfer_msg_res(
        num, response_result::unknown_error, &m_response_message);
    MockFunction<void(buffer &&, std::function<void(buffer &&)> &&)> request_handler;
    EXPECT_CALL(request_handler, Call)
        .Times(1)
        .WillOnce([&](buffer &&buf, auto &&response_handler) {
                      EXPECT_EQ(buf, transfer_req_message);
                      response_handler(transfer_res_message.copy());
                  });
    EXPECT_CALL(*m_transport_ptr, send_async)
        .Times(1)
        .WillOnce([&](buffer &&buf, auto &&error_handler) {
                      EXPECT_EQ(buf, transfer_res_message);
                      EXPECT_FALSE(error_handler);
                      sync_event.set();
                  });
    auto sut = create_sut();
    sut->set_request_handler(request_handler.AsStdFunction());

    m_transport_ptr->emit_recv_event(transfer_req_message.copy());
    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(Connection, SetsDisconnectHandler)
{
    event sync_event;
    MockFunction<void(connection_state)> disconnect_handler;
    EXPECT_CALL(disconnect_handler, Call(connection_state::disconnected))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    auto sut = create_sut();
    sut->set_change_state_handler(disconnect_handler.AsStdFunction());

    m_transport_ptr->emit_disconnect_event();
    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(Connection, CheckIfConnected)
{
    EXPECT_TRUE(m_transport_ptr->is_stopped());
    auto sut = create_sut();
    EXPECT_FALSE(m_transport_ptr->is_stopped());

    sut->stop_transport();

    ASSERT_FALSE(sut->is_active());
    ASSERT_TRUE(m_transport_ptr->is_stopped());
}

TEST_F(Connection, CallsTransportStopBeforeDestruction)
{
    auto sut = create_sut();
    sut.reset();
    
    ASSERT_TRUE(transport_mock::was_stopped);
}

TEST_F(Connection, CancelsUnfinishedRequestOnDestroy)
{
    event sync_event;
    const uint64_t num = 34;
    auto transfer_req_message = create_transfer_msg_req(num, 7, &m_request_message);
    MockFunction<void(request_result, buffer &&)> mock_function;
    EXPECT_CALL(mock_function, Call(request_result::canceled, _))
        .Times(1)
        .WillOnce([&sync_event]() { sync_event.set(); });

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), mock_function.AsStdFunction());

    sut.reset();
    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(Connection, CancelsActiveRequestsWhenDisconnectedEventReceived)
{
    event sync_event;
    auto transfer_req_message = create_transfer_msg_req(34, 7, &m_request_message);
    MockFunction<void(request_result, buffer &&)> request_callback;
    EXPECT_CALL(request_callback, Call(request_result::canceled, _))
        .Times(1)
        .WillOnce([&sync_event]() { sync_event.set(); });

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), request_callback.AsStdFunction());
    EXPECT_EQ(sut->get_active_requests_count(), 1);
    m_transport_ptr->emit_disconnect_event();

    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
    ASSERT_EQ(sut->get_active_requests_count(), 0);
}

TEST_F(Connection, ProcessCorrectlyDisconnectedEventIfHandlerIsNotSet)
{
    auto sut = create_sut();
    m_transport_ptr->emit_disconnect_event();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

TEST_F(Connection, CallsTransportStartOnSettingTransport)
{
    auto sut = create_sut_without_transport();
    EXPECT_FALSE(transport_mock::was_started);

    sut->set_and_start_transport(std::move(m_transport));

    EXPECT_TRUE(transport_mock::was_started);
}

TEST_F(Connection, CallsTransportStartCallbackOnSettingTransport)
{
    event sync_event;
    MockFunction<void(connection_state)> connect_handler;
    EXPECT_CALL(connect_handler, Call(connection_state::connected))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    auto sut = create_sut_without_transport();
    sut->set_change_state_handler(connect_handler.AsStdFunction());

    sut->set_and_start_transport(std::move(m_transport));

    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(Connection, CallsRequestHandlerWithSendErrorCodeIfNoTransport)
{
    event sync_event;
    MockFunction<void(request_result, buffer &&)> request_callback;
    EXPECT_CALL(request_callback, Call(request_result::send_error, _))
        .Times(1)
        .WillOnce([&sync_event]() { sync_event.set(); });
    auto req_message = create_transfer_msg_req(3, 7, &m_request_message);

    auto sut = create_sut_without_transport();

    sut->request_async(std::move(req_message), request_callback.AsStdFunction());
    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
    EXPECT_EQ(sut->get_active_requests_count(), 0);
}

TEST_F(Connection, AnswersFalseOnCheckIsConnectedIfNoTransport)
{
    auto sut = create_sut_without_transport();

    ASSERT_FALSE(sut->is_active());
}

TEST_F(Connection, DoesNothingOnDisconnectIfNoTransport)
{
    auto sut = create_sut_without_transport();

    ASSERT_NO_THROW(sut->stop_transport());
}

TEST_F(Connection, DoesNotCallStartCallbackIfItIsNotSet)
{
    NiceMock<MockFunction<void()>> callback;
    EXPECT_CALL(callback, Call)
        .Times(0);
    m_transport_ptr->set_start_callback(callback.AsStdFunction());
    auto sut = create_sut();
}

TEST_F(Connection, DoesNotCallStopCallbackIfItIsNotSet)
{
    NiceMock<MockFunction<void()>> callback;
    EXPECT_CALL(callback, Call)
        .Times(0);
    m_transport_ptr->set_stop_callback(callback.AsStdFunction());
    auto sut = create_sut();
    sut->stop_transport();
}
