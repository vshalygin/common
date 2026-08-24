#include <rpc-lib/internal/controller/request-controller.h>
#include <rpc-lib/closure-guard/closure-guard.h>

#include <common-lib/thread/thread-pool/thread-pool.h>

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#pragma warning(pop)

#include <gtest/gtest.h>

using namespace vshalygin;
using namespace vshalygin::rpc;
using namespace vshalygin::rpc::internal;
using namespace vshalygin::cl;
using namespace testing;

TEST(RequestController, ExecutesCallbackOnDone)
{
    thread_pool pool(2);
    auto response = std::make_unique<proto::data_message>();
    bool is_called = false;
    promise promise(&pool, [&](request_result r, std::unique_ptr<proto::data_message> m) {
                               is_called = true;
                               return ftuple(r, std::move(m));
                           });
    auto future = promise.get_future();

    {
        auto sut = request_controller<proto::data_message>::create_on_heap(
                                                                std::move(promise), std::move(response));
        closure_guard g(sut);
    }
   
    future.get();
    ASSERT_TRUE(is_called);
}

TEST(RequestController, ExecutesCallbackWithRequestResultCode)
{
    thread_pool pool(2);
    auto response = std::make_unique<proto::data_message>();
    promise promise(&pool, [&](request_result r, std::unique_ptr<proto::data_message> m) {
        EXPECT_EQ(request_result::response_parse_error, r);
        return ftuple(r, std::move(m));
    });
    auto future = promise.get_future();

    auto sut = request_controller<proto::data_message>::create_on_heap(std::move(promise), std::move(response));
    sut->set_result(request_result::response_parse_error);
    sut->Run();

    future.get();
}

TEST(RequestController, ExecutesCallbackWithResponseMessage)
{
    thread_pool pool(2);
    auto response = std::make_unique<proto::data_message>();
    auto response_ptr = response.get();
    promise promise(&pool, [&](request_result r, std::unique_ptr<proto::data_message> m) {
        EXPECT_EQ(response_ptr, m.get());
        return ftuple(r, std::move(m));
    });
    auto future = promise.get_future();

    auto sut = request_controller<proto::data_message>::create_on_heap(
        std::move(promise), std::move(response));
    sut->Run();

    future.get();
}

TEST(RequestController, RpcControllerIsCastableToIRequestController)
{
    thread_pool pool(2);
    auto response = std::make_unique<proto::data_message>();
    promise promise(&pool, [&](request_result r, std::unique_ptr<proto::data_message> m) {
        EXPECT_EQ(request_result::send_canceled, r);
        return ftuple(r, std::move(m));
    });
    auto future = promise.get_future();

    auto sut = request_controller<proto::data_message>::create_on_heap(
        std::move(promise), std::move(response));

    google::protobuf::RpcController *rpc_controller = sut;
    auto request_controller = to_request_controller(rpc_controller);
    request_controller->set_result(request_result::send_canceled);

    sut->Run();
    future.get();
}
