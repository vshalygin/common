#include <rpc-lib/internal/controller/request-controller.h>
#include <rpc-lib/closure-guard/closure-guard.h>

#include <common-lib/thread/thread-pool/thread-pool.h>

#pragma warning(push, 0)
#include <test-messages.pb.h>
#pragma warning(pop)

#include <gtest/gtest.h>

using namespace vshalygin;
using namespace vshalygin::rpc;
using namespace vshalygin::rpc::internal;
using namespace vshalygin::cl;
using namespace testing;

namespace {
    using response_promise =
        promise<thread_pool,
                ftuple<request_result, std::unique_ptr<proto::data_message>>>;
}

TEST(RequestController, ExecutesCallbackOnDone)
{
    thread_pool pool(2);
    auto response = std::make_unique<proto::data_message>();
    bool is_called = false;
    response_promise promise(&pool);
    auto future = promise.get_future();
    auto callback_future = future.then([&](auto value) {
        auto locked_value = value.lock();
        locked_value.with([&](request_result,
                              std::unique_ptr<proto::data_message> &) {
            is_called = true;
        });
    });

    {
        auto sut = request_controller<proto::data_message>::create_on_heap(
                                                                std::move(promise), std::move(response));
        closure_guard g(sut);
    }
   
    callback_future.get();
    ASSERT_TRUE(is_called);
}

TEST(RequestController, ExecutesCallbackWithRequestResultCode)
{
    thread_pool pool(2);
    auto response = std::make_unique<proto::data_message>();
    response_promise promise(&pool);
    auto future = promise.get_future();
    auto callback_future = future.then([](auto value) {
        auto locked_value = value.lock();
        locked_value.with([](request_result result,
                             std::unique_ptr<proto::data_message> &) {
            EXPECT_EQ(request_result::response_parse_error, result);
        });
    });

    auto sut = request_controller<proto::data_message>::create_on_heap(std::move(promise), std::move(response));
    sut->set_result(request_result::response_parse_error);
    sut->Run();

    callback_future.get();
}

TEST(RequestController, ExecutesCallbackWithResponseMessage)
{
    thread_pool pool(2);
    auto response = std::make_unique<proto::data_message>();
    auto response_ptr = response.get();
    response_promise promise(&pool);
    auto future = promise.get_future();
    auto callback_future = future.then([response_ptr](auto value) {
        auto locked_value = value.lock();
        locked_value.with([response_ptr](request_result,
                                         std::unique_ptr<proto::data_message> &message) {
            EXPECT_EQ(response_ptr, message.get());
        });
    });

    auto sut = request_controller<proto::data_message>::create_on_heap(
        std::move(promise), std::move(response));
    sut->Run();

    callback_future.get();
}

TEST(RequestController, RpcControllerIsCastableToIRequestController)
{
    thread_pool pool(2);
    auto response = std::make_unique<proto::data_message>();
    response_promise promise(&pool);
    auto future = promise.get_future();
    auto callback_future = future.then([](auto value) {
        auto locked_value = value.lock();
        locked_value.with([](request_result result,
                             std::unique_ptr<proto::data_message> &) {
            EXPECT_EQ(request_result::send_canceled, result);
        });
    });

    auto sut = request_controller<proto::data_message>::create_on_heap(
        std::move(promise), std::move(response));

    google::protobuf::RpcController *rpc_controller = sut;
    auto request_controller = to_request_controller(rpc_controller);
    request_controller->set_result(request_result::send_canceled);

    sut->Run();
    callback_future.get();
}
