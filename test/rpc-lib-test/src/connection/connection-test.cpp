#include <rpc-lib/connection/connection.h>
#include <rpc-lib/transfer-message/transfer-message.h>

#include "mocks/multiple-timer-mock.h"

#include "proto/test-messages.pb.h"

#include <list>

using namespace vsh::cl;
using namespace vsh::rpc;
using namespace testing;

namespace {
    class transport_mock
        : public itransport
    {
    public:
        MOCK_METHOD(void, send_async, (buffer &&message,
                                       std::function<void()> &&error_handler), (const, override));

        void recv_async(std::function<void(buffer &&)> &&handler) const override
        {
            m_recv_handler = std::move(handler);
        }

        void stop()
        {
            m_is_stopped = true;
            if(m_stop_handler) {
                m_stop_handler();
            }
        }

        bool is_stopped() const
        {
            return m_is_stopped;
        }

        void set_stop_handler(std::function<void()> &&handler)
        {
            m_stop_handler = std::move(handler);
        }

        void emit_recv_event(buffer &&message)
        {
            ASSERT_TRUE(m_recv_handler) << "recv_async was never called";
            m_recv_handler(std::move(message));
        }

    private:
        bool m_is_stopped = false;
        mutable std::function<void(buffer &&)> m_recv_handler;
        std::function<void()> m_stop_handler;
    };

    using transport_nice_mock = ::NiceMock<transport_mock>;
}

class Connection
    : public Test
{
protected:
    void SetUp() override
    {
        m_transport = std::make_unique<transport_nice_mock>();
        m_multiple_timer = std::make_unique<multiple_timer_nice_mock>();

        m_transport_ptr = m_transport.get();
        m_multiple_timer_ptr = m_multiple_timer.get();

        m_request_message.set_some_data(34);
        m_response_message.set_some_data(43);
    }

    std::unique_ptr<iconnection> create_sut()
    {
        return std::make_unique<connection>(std::move(m_transport), std::move(m_multiple_timer));
    }

protected:
    std::unique_ptr<transport_nice_mock> m_transport;
    std::unique_ptr<multiple_timer_nice_mock> m_multiple_timer;

    transport_nice_mock *m_transport_ptr;
    multiple_timer_nice_mock  *m_multiple_timer_ptr;

    proto::some_message m_request_message;
    proto::some_message m_response_message;
};

TEST_F(Connection, LaunchesTimerOnRequestAsyncOperation)
{
    EXPECT_CALL(*m_multiple_timer, start(_, std::chrono::microseconds(10000000)))
        .Times(1);
    auto req_message = create_transfer_msg_req(3, 7, &m_request_message);
    auto sut = create_sut();

    sut->request_async(std::move(req_message), {});
}

TEST_F(Connection, LaunchesRequestAsyncOperation)
{
    auto req_message = create_transfer_msg_req(3, 7, &m_request_message);
    EXPECT_CALL(*m_transport, send_async)
        .Times(1)
        .WillOnce([req_message = req_message.copy()](auto buf, auto) { EXPECT_EQ(req_message, buf); });

    auto sut = create_sut();
    sut->request_async(std::move(req_message), {});
}

TEST_F(Connection, ProcessesZeroRequestAfterCreation)
{
    auto sut = create_sut();

    ASSERT_EQ(sut->get_processing_requests_count(), 0);
}

TEST_F(Connection, AddsProcessingRequestAfterReqAsyncCalled)
{
    auto req_message = create_transfer_msg_req(3, 7, &m_request_message);
    auto sut = create_sut();
    sut->request_async(std::move(req_message), {});

    ASSERT_EQ(sut->get_processing_requests_count(), 1);
}

TEST_F(Connection, DoNotCallEmptyCallbackOnRequestAsyncOperation)
{
    auto transfer_req_message = create_transfer_msg_req(3, 7, &m_request_message);
    auto transfer_res_message = create_transfer_msg_res(
        3, response_result::rejected, &m_response_message);

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), {});
    m_transport_ptr->emit_recv_event(std::move(transfer_res_message));
}

TEST_F(Connection, ClearsRequestAsyncOperationAfterItsCompleted)
{
    auto transfer_req_message = create_transfer_msg_req(3, 7, &m_request_message);
    auto transfer_res_message = create_transfer_msg_res(
        3, response_result::rejected, &m_response_message);

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), {});
    m_transport_ptr->emit_recv_event(std::move(transfer_res_message));

    ASSERT_EQ(sut->get_processing_requests_count(), 0);
}

TEST_F(Connection, ProcessesResponseToRequestAsync)
{
    auto transfer_req_message = create_transfer_msg_req(3, 7, &m_request_message);
    auto transfer_res_message = create_transfer_msg_res(
        3, response_result::rejected, &m_response_message);
    MockFunction<void(request_result, buffer &&)> mock_function;
    EXPECT_CALL(mock_function, Call(request_result::ok, _))
        .Times(1)
        .WillOnce([transfer_res_msg = transfer_res_message.copy()](request_result, buffer &&message) {
                      EXPECT_EQ(transfer_res_msg, message);
                  });

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), mock_function.AsStdFunction());
    m_transport_ptr->emit_recv_event(std::move(transfer_res_message));
}

TEST_F(Connection, CallsRequestCallbackOnlyOnCorrespondingResponse)
{
    const uint64_t num = 34;

    auto transfer_req_message = create_transfer_msg_req(num, 7, &m_request_message);
    auto transfer_res_message = create_transfer_msg_res(
        num+1, response_result::rejected, &m_response_message);
    MockFunction<void(request_result, buffer &&)> mock_function;
    EXPECT_CALL(mock_function, Call)
        .Times(0);

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), mock_function.AsStdFunction());
    m_transport_ptr->emit_recv_event(std::move(transfer_res_message));
}

TEST_F(Connection, CatchesExceptionsThrownByRequestCallback)
{
    const uint64_t num = 34;

    auto transfer_req_message = create_transfer_msg_req(num, 7, &m_request_message);
    auto transfer_res_message = create_transfer_msg_res(
        num, response_result::rejected, &m_response_message);
    MockFunction<void(request_result, buffer &&)> mock_function;
    EXPECT_CALL(mock_function, Call)
        .Times(1)
        .WillOnce(Throw(std::exception()));

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), mock_function.AsStdFunction());
    
    ASSERT_NO_THROW(m_transport_ptr->emit_recv_event(std::move(transfer_res_message)));
    ASSERT_EQ(sut->get_processing_requests_count(), 0);
}

TEST_F(Connection, CancelsTimerAfterRequestCallbackInvoked)
{
    const uint64_t timer_id = 546456;
    EXPECT_CALL(*m_multiple_timer, start)
        .Times(1)
        .WillOnce(Return(timer_id));
    EXPECT_CALL(*m_multiple_timer, cancel(timer_id))
        .Times(1);
    const uint64_t num = 34;
    auto transfer_req_message = create_transfer_msg_req(num, 7, &m_request_message);
    auto transfer_res_message = create_transfer_msg_res(
        num, response_result::rejected, &m_response_message);

    auto sut = create_sut();
    sut->request_async(std::move(transfer_req_message), {});

    m_transport_ptr->emit_recv_event(std::move(transfer_res_message));
}