#include <rpc-lib/service/response-callback/response-callback.h>
#include <rpc-lib/closure-guard/closure-guard.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::rpc;
using namespace testing;

TEST(ResponseCallback, CallsCallbackOnRunMethod)
{
    NiceMock<MockFunction<void(response_result)>> callback;
    EXPECT_CALL(callback, Call)
        .Times(1);
    auto sut = response_callback::create_on_heap(callback.AsStdFunction());
    closure_guard cg(sut);
}

TEST(ResponseCallback, CatchesExceptionsInCallback)
{
    NiceMock<MockFunction<void(response_result)>> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce(Throw(std::exception()));
    auto sut = response_callback::create_on_heap(callback.AsStdFunction());
    ASSERT_NO_THROW(sut->Run());
}

TEST(ResponseCallback, SetsErrorCodeWhichWillBeUsedInCallback)
{
    NiceMock<MockFunction<void(response_result)>> callback;
    EXPECT_CALL(callback, Call(response_result::unknown_error))
        .Times(1);

    auto sut = response_callback::create_on_heap(callback.AsStdFunction());
    sut->SetFailed(to_string(response_result::unknown_error));
    closure_guard cg(sut);
}

TEST(ResponseCallback, AnswersSetError)
{
    NiceMock<MockFunction<void(response_result)>> callback;

    auto sut = response_callback::create_on_heap(callback.AsStdFunction());
    sut->SetFailed(to_string(response_result::unknown_error));
    closure_guard cg(sut);

    ASSERT_EQ(to_string(response_result::unknown_error), sut->ErrorText());
}

TEST(ResponseCallback, AnswersTrueOnCheckIfCanceled)
{
    NiceMock<MockFunction<void(response_result)>> callback;

    auto sut = response_callback::create_on_heap(callback.AsStdFunction());
    closure_guard cg(sut);
    sut->SetFailed(to_string(response_result::canceled));

    ASSERT_TRUE(sut->IsCanceled());
}

TEST(ResponseCallback, AnswersFalseOnCheckIfNotCanceled)
{
    NiceMock<MockFunction<void(response_result)>> callback;

    auto sut = response_callback::create_on_heap(callback.AsStdFunction());
    closure_guard cg(sut);

    ASSERT_FALSE(sut->IsCanceled());
}

TEST(ResponseCallback, AnswersTrueOnCheckIfFailed)
{
    NiceMock<MockFunction<void(response_result)>> callback;

    auto sut = response_callback::create_on_heap(callback.AsStdFunction());
    closure_guard cg(sut);
    sut->SetFailed(to_string(response_result::unknown_error));

    ASSERT_TRUE(sut->Failed());
}

TEST(ResponseCallback, AnswersFalseOnCheckIfNotFailed)
{
    NiceMock<MockFunction<void(response_result)>> callback;

    auto sut = response_callback::create_on_heap(callback.AsStdFunction());
    closure_guard cg(sut);

    ASSERT_FALSE(sut->Failed());
}

TEST(ResponseCallback, CallsCallbackWithUnknownErrorParameterIfDestroyedByException)
{
    try {
        NiceMock<MockFunction<void(response_result)>> callback;
        EXPECT_CALL(callback, Call(response_result::unknown_error))
            .Times(1);

        auto sut = response_callback::create_on_heap(callback.AsStdFunction());
        closure_guard cg(sut);
        throw std::runtime_error("");
    } catch(...) {
    }
}
