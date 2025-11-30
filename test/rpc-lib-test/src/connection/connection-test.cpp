#include <rpc-lib/connection/connection.h>
#include <rpc-lib/transfer-message/transfer-message.h>

#include "mocks/multiple-timer-mock.h"
#include "mocks/transport-mock.h"

#include "proto/test-messages.pb.h"

using namespace vsh::cl;
using namespace vsh::rpc;
using namespace testing;

class Connection
    : public Test
{
protected:
    void SetUp() override
    {
        m_transport = std::make_unique<transport_nice_mock>();
        m_multiple_timer = std::make_unique<multiple_timer_nice_mock>();

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

    proto::some_message m_request_message;
    proto::some_message m_response_message;
};

TEST_F(Connection, LaunchesTimerOnRequestAsyncOperation)
{
    EXPECT_CALL(*m_multiple_timer, start(_, std::chrono::microseconds(10000000)))
        .Times(1);
    auto sut = create_sut();

    sut->request_async({}, {});
}

TEST_F(Connection, LaunchesRequestAsyncOperation)
{
    auto req_message = create_transfer_msg_req(3, 7, &m_request_message);
    EXPECT_CALL(*m_transport, send_async(req_message.copy(), _))
        .Times(1);
    auto sut = create_sut();

    sut->request_async(std::move(req_message), {});
}
