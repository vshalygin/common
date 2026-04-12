#include <rpc-lib/service/service.h>
#include <rpc-lib/closure-guard/closure-guard.h>

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#include <google/protobuf/util/message_differencer.h>
#pragma warning(pop)

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::rpc;
using namespace vshalygin::cl;
using namespace testing;
using namespace google::protobuf::util;

namespace {
    class ProtoServiceMock
        : public proto::Service
    {
    public:
        ProtoServiceMock() = default;

        MOCK_METHOD(void, Method, (::google::protobuf::RpcController *controller,
                                   const ::proto::request_message *request,
                                   ::proto::response_message *response,
                                   ::google::protobuf::Closure *done), (override));
        MOCK_METHOD(void, Method2, (::google::protobuf::RpcController *controller,
                                    const ::proto::request_message *request,
                                    ::proto::response_message *response,
                                    ::google::protobuf::Closure *done), (override));
    };

    using ProtoServiceNiceMock = NiceMock<ProtoServiceMock>;
}

class Service
    : public Test
{
protected:
    void SetUp() override
    {
        auto gservice = std::make_unique<ProtoServiceNiceMock>();
        m_gservice = gservice.get();
        m_service = std::make_unique<service<ProtoServiceMock>>(std::move(gservice));

        m_request_message.set_data(34);
        m_response_message.set_data(44);
        m_response_message.mutable_data3()->set_string_data("sfdsdfsdfsdaafdasdfs");
        m_some_message.set_string_data2("s3dswf4sdf00");
    }

protected:
    std::unique_ptr<iservice> m_service;
    ProtoServiceNiceMock *m_gservice;

    proto::request_message m_request_message;
    proto::response_message m_response_message;
    proto::some_message m_some_message;
};

TEST_F(Service, CallsResponseCallbackWithRequestParseErrorCodeIfParsingFailed)
{
    m_response_message.Clear();
    auto response_buf = create_transfer_msg_res(34,
                                                response_result::request_parse_error,
                                                &m_response_message);
    auto invalid_request_buf = create_transfer_msg_req(34,
                                                       1,
                                                       &m_some_message);
    MockFunction<void(buffer &&)> response_callback;
    EXPECT_CALL(response_callback, Call)
        .Times(1)
        .WillOnce([&](auto &&buf) { EXPECT_EQ(response_buf, buf); });
    EXPECT_CALL(*m_gservice, Method2)
        .Times(0);

    m_service->process_request(std::move(invalid_request_buf), response_callback.AsStdFunction());
}

TEST_F(Service, CallsResponseCallbackWithResponseTooBigCode)
{
    proto::response_message empty_response;
    m_response_message.mutable_data3()->set_string_data2(std::string(8 * 1024 * 1024 + 1, 'a'));
    auto response_buf = create_transfer_msg_res(34,
                                                response_result::response_too_big,
                                                &empty_response);
    auto request_buf = create_transfer_msg_req(34,
                                               1,
                                               &m_request_message);
    MockFunction<void(buffer &&)> response_callback;
    EXPECT_CALL(response_callback, Call)
        .Times(1)
        .WillOnce([&](auto &&buf) { EXPECT_EQ(response_buf, buf); });
    EXPECT_CALL(*m_gservice, Method2)
        .Times(1)
        .WillOnce([&](::google::protobuf::RpcController * /*controller*/,
                      const ::proto::request_message *request,
                      ::proto::response_message *response,
                      ::google::protobuf::Closure *done) {
                      closure_guard cg(done);
                      EXPECT_TRUE(MessageDifferencer::Equals(m_request_message, *request));
                      response->CopyFrom(m_response_message);
                  });

    m_service->process_request(std::move(request_buf), response_callback.AsStdFunction());
}

TEST_F(Service, CallsResponseCallbackWithSetResponseErrorCode)
{
    auto response_buf = create_transfer_msg_res(34,
                                                response_result::unknown_error,
                                                &m_response_message);
    auto request_buf = create_transfer_msg_req(34,
                                               1,
                                               &m_request_message);
    MockFunction<void(buffer &&)> response_callback;
    EXPECT_CALL(response_callback, Call)
        .Times(1)
        .WillOnce([&](auto &&buf) { EXPECT_EQ(response_buf, buf); });
    EXPECT_CALL(*m_gservice, Method2)
        .Times(1)
        .WillOnce([&](::google::protobuf::RpcController *controller,
                  const ::proto::request_message *request,
                  ::proto::response_message *response,
                  ::google::protobuf::Closure *done) {
                      closure_guard cg(done);
                      EXPECT_TRUE(MessageDifferencer::Equals(m_request_message, *request));
                      controller->SetFailed(to_string(response_result::unknown_error));
                      response->CopyFrom(m_response_message);
                  });

    m_service->process_request(std::move(request_buf), response_callback.AsStdFunction());
}

TEST_F(Service, CallsResponseCallbackWithNotImplementedErrorCodeIfGServiceWasNotSet)
{
    service<ProtoServiceMock> sut(nullptr);
    m_response_message.Clear();
    auto response_buf = create_transfer_msg_res(34,
                                                response_result::not_implemented,
                                                &m_response_message);
    auto request_buf = create_transfer_msg_req(34,
                                               1,
                                               &m_request_message);
    MockFunction<void(buffer &&)> response_callback;
    EXPECT_CALL(response_callback, Call)
        .Times(1)
        .WillOnce([&](auto &&buf) { EXPECT_EQ(response_buf, buf); });
    EXPECT_CALL(*m_gservice, Method2)
        .Times(0);

    sut.process_request(std::move(request_buf), response_callback.AsStdFunction());
}

TEST_F(Service, CallsResponseCallbackWithNotImplementedErrorCodeIfMethodIdxEqualsToMethodsCounst)
{
    m_response_message.Clear();
    auto response_buf = create_transfer_msg_res(34,
                                                response_result::not_implemented,
                                                &m_response_message);
    auto request_buf = create_transfer_msg_req(34,
                                               m_gservice->descriptor()->method_count(),
                                               &m_request_message);
    MockFunction<void(buffer &&)> response_callback;
    EXPECT_CALL(response_callback, Call)
        .Times(1)
        .WillOnce([&](auto &&buf) { EXPECT_EQ(response_buf, buf); });
    EXPECT_CALL(*m_gservice, Method2)
        .Times(0);

    m_service->process_request(std::move(request_buf), response_callback.AsStdFunction());
}