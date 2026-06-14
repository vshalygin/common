#include <rpc-lib/connector/connector.h>
#include <rpc-lib/transport/itransport.h>

#include "mocks/pipe-mock.h"
#include "mocks/pipe-env-mock.h"
#include "mocks/authenticator-mock.h"

#pragma warning(push, 0)
#include <google/protobuf/util/message_differencer.h>
#pragma warning(pop)

#include <gtest/gtest.h>

using namespace vshalygin::rpc;
using namespace vshalygin::cl;

using namespace testing;
using namespace google::protobuf::util;

class Connector
    : public Test
{
protected:
    void SetUp() override
    {
        m_req.set_auth_data("sdfadfadsfsdafdd asdf");
        m_res.set_is_accepted(true);

        m_pipe = std::make_shared<pipe_nice_mock>();
        m_pipe_env = std::make_shared<pipe_env_nice_mock>();
        m_authenticator = std::make_shared<authenticator_nice_mock>();

        m_sut = std::make_unique<connector>(m_pipe_env, m_authenticator);

        ON_CALL(*m_pipe, is_connected)
            .WillByDefault(Return(true));
        ON_CALL(*m_pipe, wait_connect_for)
            .WillByDefault(Return(true));
        ON_CALL(*m_pipe_env, open_pipe)
            .WillByDefault(Return(m_pipe));
        ON_CALL(*m_pipe, try_to_write_for)
            .WillByDefault([this](buffer &&msg, const std::chrono::microseconds &) {
                               proto::auth_request req;
                               req.ParseFromArray(msg.data(), static_cast<int>(msg.size()));
                               EXPECT_TRUE(MessageDifferencer::Equals(m_req, req));
                               return true;
                           });
        ON_CALL(*m_pipe, try_to_read_for)
            .WillByDefault([this]() {
                               buffer buf(m_res.ByteSizeLong());
                               m_res.SerializeToArray(buf.data(), static_cast<int>(buf.size()));
                               return buf;
                           });
        ON_CALL(*m_authenticator, create_request)
            .WillByDefault([this]() { return m_req; });
    }

protected:
    proto::auth_response m_res;
    proto::auth_request m_req;

    std::shared_ptr<pipe_env_nice_mock> m_pipe_env;
    std::shared_ptr<pipe_nice_mock> m_pipe;
    std::shared_ptr<authenticator_nice_mock> m_authenticator;

    std::unique_ptr<iconnector> m_sut;
};

TEST_F(Connector, CreatesTransport)
{
    EXPECT_CALL(*m_pipe, write_async)
        .Times(1)
        .WillOnce(Return(true));

    auto transport = m_sut->create_transport();

    transport->send_async({}, {});
}

TEST_F(Connector, ThrowsExceptionIfFailedToWaitPipeConnect)
{
    EXPECT_CALL(*m_pipe, wait_connect_for)
        .Times(1)
        .WillOnce(Return(false));

    ASSERT_ANY_THROW(m_sut->create_transport());
}

TEST_F(Connector, ThrowsExceptionIfWritingReqMessageFailed)
{
    EXPECT_CALL(*m_pipe, try_to_write_for)
        .Times(1)
        .WillOnce(Return(false));

    ASSERT_ANY_THROW(m_sut->create_transport());
}

TEST_F(Connector, ThrowsExceptionIfReadingResMessageFailed)
{
    EXPECT_CALL(*m_pipe, try_to_read_for)
        .Times(1)
        .WillOnce(Return(std::nullopt));

    ASSERT_ANY_THROW(m_sut->create_transport());
}

TEST_F(Connector, ThrowsExceptionIfFailedToParseResponse)
{
    buffer invalid_buff(4);
    invalid_buff[0] = std::byte(3);
    invalid_buff[0] = std::byte(14);
    invalid_buff[0] = std::byte(2);
    invalid_buff[0] = std::byte(34);
    EXPECT_CALL(*m_pipe, try_to_read_for)
        .Times(1)
        .WillOnce(Return(std::optional(invalid_buff.copy())));

    ASSERT_ANY_THROW(m_sut->create_transport());
}

TEST_F(Connector, ThrowsExceptionIfServerDidNotExceptConnection)
{
    m_res.set_is_accepted(false);

    ASSERT_ANY_THROW(m_sut->create_transport());
}
