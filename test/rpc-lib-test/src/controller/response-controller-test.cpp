#include <rpc-lib/controller/response-controller.h>
#include <rpc-lib/closure-guard/closure-guard.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::rpc;
using namespace testing;

TEST(ResponseController, CallsCallbackOnRunMethod)
{
    NiceMock<MockFunction<void(response_result)>> callback;
    EXPECT_CALL(callback, Call)
        .Times(1);
    auto sut = response_controller<decltype(callback.AsStdFunction())>::
                                               create_on_heap(callback.AsStdFunction(), 0);
    closure_guard cg(sut);
}

TEST(ResponseController, CatchesExceptionsInCallback)
{
    NiceMock<MockFunction<void(response_result)>> callback;
    EXPECT_CALL(callback, Call)
        .Times(1)
        .WillOnce(Throw(std::exception()));
    auto sut = response_controller<decltype(callback.AsStdFunction())>::
                                              create_on_heap(callback.AsStdFunction(), 0);
    ASSERT_NO_THROW(sut->Run());
}

TEST(ResponseController, SetsErrorCodeWhichWillBeUsedInCallback)
{
    NiceMock<MockFunction<void(response_result)>> callback;
    EXPECT_CALL(callback, Call(response_result::request_parse_error))
        .Times(1);

    auto sut = response_controller<decltype(callback.AsStdFunction())>::
                                              create_on_heap(callback.AsStdFunction(), 0);
    sut->set_response_result(response_result::request_parse_error);
    closure_guard cg(sut);
}

TEST(ResponseController, CallsCallbackWithUnknownErrorParameterIfDestroyedByException)
{
    try {
        NiceMock<MockFunction<void(response_result)>> callback;
        EXPECT_CALL(callback, Call(response_result::unknown_error))
            .Times(1);

        auto sut = response_controller<decltype(callback.AsStdFunction())>::
                                                create_on_heap(callback.AsStdFunction(), 0);
        sut->set_response_result(response_result::ok);
        closure_guard cg(sut);
        throw std::runtime_error("");
    } catch(...) {
    }
}

TEST(ResponseController, AnswersConnectionId)
{
    NiceMock<MockFunction<void(response_result)>> callback;
    EXPECT_CALL(callback, Call)
        .Times(1);

    auto sut = response_controller<decltype(callback.AsStdFunction())>::
                                                   create_on_heap(callback.AsStdFunction(), 34);
    closure_guard cg(sut);

    ASSERT_EQ(sut->get_connection_id(), 34);
}

TEST(ResponseController, IsConvertibleToInterface)
{
    NiceMock<MockFunction<void(response_result)>> callback;
    EXPECT_CALL(callback, Call)
        .Times(1);

    auto sut = response_controller<decltype(callback.AsStdFunction())>::
        create_on_heap(callback.AsStdFunction(), 34);
    closure_guard cg(sut);
    auto iface = to_response_controller(sut);
    ASSERT_EQ(sut->get_connection_id(), iface->get_connection_id());
}

TEST(ResponseController, IsConvertibleToInterfaceEx)
{
    NiceMock<MockFunction<void(response_result)>> callback;
    EXPECT_CALL(callback, Call(response_result::response_too_big))
        .Times(1);

    auto sut = response_controller<decltype(callback.AsStdFunction())>::
        create_on_heap(callback.AsStdFunction(), 34);
    closure_guard cg(sut);
    auto iface = to_response_controller_ex(sut);
    iface->set_response_result(response_result::response_too_big);
}
