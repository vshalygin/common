#include <rpc-lib/controller/request-controller.h>
#include <rpc-lib/closure-guard/closure-guard.h>

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#pragma warning(pop)

#include <gtest/gtest.h>

using namespace vshalygin::rpc;
using namespace testing;

TEST(RequestController, ExecutesCallbackOnDone)
{
    auto response = std::make_unique<proto::data_message>();
    bool is_called = false;
    auto callback = [&](request_result, std::unique_ptr<proto::data_message>) {
        is_called = true;
    };

    {
        auto sut = request_controller<proto::data_message, decltype(callback)>::create_on_heap(
                                                                std::move(callback), std::move(response));
        closure_guard g(sut);
    }
   
    ASSERT_TRUE(is_called);
}

TEST(RequestController, ExecutesCallbackWithRequestResultCode)
{
    auto response = std::make_unique<proto::data_message>();
    auto callback = [&](request_result r, std::unique_ptr<proto::data_message> ) {
        ASSERT_EQ(request_result::response_parse_error, r);
    };

    auto sut = request_controller<proto::data_message, decltype(callback)>::create_on_heap(
        std::move(callback), std::move(response));
    closure_guard g(sut);
    sut->set_result(request_result::response_parse_error);
}

TEST(RequestController, ExecutesCallbackWithResponseMessage)
{
    auto response = std::make_unique<proto::data_message>();
    auto response_ptr = response.get();
    auto callback = [&](request_result, std::unique_ptr<proto::data_message> res) {
        ASSERT_EQ(response_ptr, res.get());
    };

    auto sut = request_controller<proto::data_message, decltype(callback)>::create_on_heap(
        std::move(callback), std::move(response));
    closure_guard g(sut);
}

TEST(RequestController, RpcControllerIsCastableToIRequestController)
{
    auto response = std::make_unique<proto::data_message>();
    auto callback = [&](request_result r, std::unique_ptr<proto::data_message>) {
        ASSERT_EQ(request_result::send_canceled_error, r);
    };

    auto sut = request_controller<proto::data_message, decltype(callback)>::create_on_heap(
        std::move(callback), std::move(response));
    closure_guard g(sut);

    google::protobuf::RpcController *rpc_controller = sut;
    auto request_controller = to_request_controller(rpc_controller);
    request_controller->set_result(request_result::send_canceled_error);
}
