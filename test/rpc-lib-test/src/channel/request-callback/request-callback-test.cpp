#include <rpc-lib/channel/request-callback/request-callback.h>
#include <rpc-lib/channel/closure-guard/closure-guard.h>

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#pragma warning(pop)

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vsh::rpc;
using namespace testing;

class RequestCallback
    : public Test
{
protected:
    using Request = proto::some_message;
    using Response = proto::some_message;

protected:
    RequestCallback()
        : m_guard(nullptr)
    {};

    void SetUp() override
    {
        m_callback_ptr = &m_callback;

        m_request_callback = request_callback<Response>::create_on_heap(m_callback.AsStdFunction());
        m_guard = closure_guard(m_request_callback);
    }

protected:
    NiceMock<MockFunction<void(request_result, std::unique_ptr<Response>)>> *m_callback_ptr;

    closure_guard m_guard;
    request_callback<Response> *m_request_callback;

private:
    NiceMock<MockFunction<void(request_result, std::unique_ptr<Response>)>> m_callback;
};

TEST_F(RequestCallback, AnswersResponseNotNullPtr)
{
    ASSERT_THAT(m_request_callback->get_response_ptr(), NotNull());
}

TEST_F(RequestCallback, CallsCallbackWithResponseParameter)
{
    auto response_ptr = m_request_callback->get_response_ptr();
    EXPECT_CALL(*m_callback_ptr, Call)
        .Times(1)
        .WillOnce([response_ptr](request_result, std::unique_ptr<Response> response) {
                      EXPECT_EQ(response.get(), response_ptr);
                  });
    
    m_guard.reset();
    Mock::VerifyAndClearExpectations(m_callback_ptr);
}

TEST_F(RequestCallback, IsNotFailedByDefault)
{
    ASSERT_FALSE(m_request_callback->Failed());
}

TEST_F(RequestCallback, IsFailedAfterSetFailed)
{
    m_request_callback->SetFailed(to_string(request_result::unknown_error));

    ASSERT_TRUE(m_request_callback->Failed());
}

TEST_F(RequestCallback, AnswersErrorText)
{
    m_request_callback->SetFailed(to_string(request_result::unknown_error));

    ASSERT_EQ(m_request_callback->ErrorText(), to_string(request_result::unknown_error));
}

TEST_F(RequestCallback, AnswersTrueIfCanceled)
{
    m_request_callback->SetFailed(to_string(request_result::canceled));

    ASSERT_TRUE(m_request_callback->IsCanceled());
}

TEST_F(RequestCallback, AnswersFalseIfNotCanceled)
{
    ASSERT_FALSE(m_request_callback->IsCanceled());
}
