#include <rpc-lib/channel/channel.h>
#include <rpc-lib/transfer-message/transfer-message.h>
#include <rpc-lib/controller/request-controller.h>
#include <rpc-lib/closure-guard/closure-guard.h>

#include "mocks/connection-mock.h"

#include <common-lib/synchronization/event/event.h>

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#include <google/protobuf/util/message_differencer.h>
#pragma warning(pop)

#include <gtest/gtest.h>

using namespace vshalygin;
using namespace vshalygin::rpc;
using namespace vshalygin::cl;
using namespace testing;
using namespace google::protobuf::util;

namespace {
    class Service
        : public proto::Service
    {};
}

class ChannelBase
    : public Test
{
protected:
    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(2);
    }

    void TearDown() override
    {
        m_thread_pool->stop();
    }

protected:
    MockFunction<void(request_result, std::unique_ptr<proto::response_message>)> m_request_callback;

    std::shared_ptr<thread_pool> m_thread_pool;
};

class Channel
    : public ChannelBase
{
protected:
    void SetUp() override
    {
        ChannelBase::SetUp();

        auto connection = std::make_unique<connection_nice_mock>();
        m_connection_ptr = connection.get();
        m_sut = std::make_unique<channel>(std::move(connection));

        m_request_message.set_data(45);   
        m_some_message.set_string_data2("sdfasdfasdfsadfasdfsad");

        m_response_message = std::make_unique<proto::response_message>();
        m_response_message_ptr = m_response_message.get();
        m_response_message_ptr->set_data2(34);
    }

    void call_method()
    {
        auto req_controller =
            request_controller<proto::response_message, decltype(m_request_callback.AsStdFunction())>
            ::create_on_heap(m_request_callback.AsStdFunction(), std::move(m_response_message));

        m_closure = req_controller;
        m_rpc_controller = req_controller;


        m_sut->CallMethod(m_service.descriptor()->method(0),
                          m_rpc_controller,
                          &m_request_message,
                          m_response_message_ptr,
                          m_closure);
    }

    void call_method2()
    {
        auto response_message2 = std::make_unique<proto::response_message>();
        m_response_message_ptr2 = response_message2.get();
        m_response_message_ptr2->set_data2(35);
        auto req_controller2 =
            request_controller<proto::response_message, decltype(m_request_callback.AsStdFunction())>
            ::create_on_heap(m_request_callback.AsStdFunction(), std::move(response_message2));

        m_closure2 = req_controller2;
        m_rpc_controller2 = req_controller2;

        m_sut->CallMethod(m_service.descriptor()->method(0),
                          m_rpc_controller2,
                          &m_request_message,
                          m_response_message_ptr2,
                          m_closure2);
    }

protected:
    Service m_service;
    google::protobuf::Closure *m_closure;
    google::protobuf::RpcController *m_rpc_controller;
    google::protobuf::Closure *m_closure2;
    google::protobuf::RpcController *m_rpc_controller2;

    connection_nice_mock *m_connection_ptr;

    proto::request_message m_request_message;
    std::unique_ptr<proto::response_message> m_response_message;
    proto::response_message *m_response_message_ptr;
    proto::response_message *m_response_message_ptr2;
    proto::some_message m_some_message;

    std::unique_ptr<channel> m_sut;
};

TEST_F(Channel, MakesRequestWithCorrectRequestMessage)
{
    event sync_event;
    EXPECT_CALL(m_request_callback, Call)
        .Times(1)
        .WillOnce([&sync_event]() { sync_event.set(); });

    auto promise = make_promise(m_thread_pool.get(), [](request_result r, buffer &&b) {
        return rpc::ftuple(r, std::move(b));
    });

    EXPECT_CALL(*m_connection_ptr, request_async)
        .Times(1)
        .WillOnce([&](auto &&buf) {
                      EXPECT_EQ(create_transfer_msg_req(0, 0, &m_request_message), buf);
                      return promise.get_future();
                  });
    
    promise.resolve(request_result::ok,
                    create_transfer_msg_res(0, response_result::ok, m_response_message_ptr));

    call_method();

    sync_event.wait();
}

TEST_F(Channel, IncrementsMessageNumberOnEveryRequest)
{
    auto promise1 = make_promise(m_thread_pool.get(), [](request_result r, buffer &&b) {
        return rpc::ftuple(r, std::move(b));
    });
    auto promise2 = make_promise(m_thread_pool.get(), [](request_result r, buffer &&b) {
        return rpc::ftuple(r, std::move(b));
    });

    EXPECT_CALL(*m_connection_ptr, request_async)
        .Times(2)
        .WillOnce([&](auto &&buf) { EXPECT_EQ(get_msg_number_req(buf), 0); return promise1.get_future(); })
        .WillOnce([&](auto &&buf) { EXPECT_EQ(get_msg_number_req(buf), 1); return promise2.get_future(); });

    call_method();
    call_method2();
}

TEST_F(Channel, SetsControllerFailedIfCallbackCalledWithErrorCode)
{
    event sync_event;
    EXPECT_CALL(m_request_callback, Call(request_result::timeout, _))
        .Times(1)
        .WillOnce([&sync_event]() { sync_event.set(); });

    auto promise = make_promise(m_thread_pool.get(), [](request_result r, buffer &&b) {
        return rpc::ftuple(r, std::move(b));
    });

    EXPECT_CALL(*m_connection_ptr, request_async)
        .Times(1)
        .WillOnce([&](auto &&) { return promise.get_future(); });

    promise.resolve(request_result::timeout, {});

    call_method();

    sync_event.wait();
}

TEST_F(Channel, SetsControllerFailedIfParsingResponseFailed)
{
    event sync_event;
    EXPECT_CALL(m_request_callback, Call(request_result::response_parse_error, _))
        .Times(1)
        .WillOnce([&sync_event]() { sync_event.set(); });

    auto promise = make_promise(m_thread_pool.get(), [](request_result r, buffer &&b) {
        return rpc::ftuple(r, std::move(b));
    });

    EXPECT_CALL(*m_connection_ptr, request_async)
        .Times(1)
        .WillOnce([&](auto &&) { return promise.get_future(); });

    promise.resolve(request_result::ok, create_transfer_msg_res(0, response_result::ok, &m_some_message));
    call_method();

    sync_event.wait();
}

TEST_F(Channel, SetsControllerFailedWithRequestNotProcessedCodeIfResponseHasFailedCode)
{
    event sync_event;
    EXPECT_CALL(m_request_callback, Call(request_result::request_not_processed, _))
        .Times(1)
        .WillOnce([&sync_event]() { sync_event.set(); });

    auto promise = make_promise(m_thread_pool.get(), [](request_result r, buffer &&b) {
        return rpc::ftuple(r, std::move(b));
    });

    EXPECT_CALL(*m_connection_ptr, request_async)
        .Times(1)
        .WillOnce([&](auto &&) { return promise.get_future(); });

    proto::response_message response_message;
    response_message.set_data(34);

    promise.resolve(request_result::ok,
                    create_transfer_msg_res(0, response_result::unknown_error, &response_message));

    call_method();

    sync_event.wait();
}

TEST_F(Channel, ParsesResponse)
{
    proto::response_message response_message_copy;
    response_message_copy.CopyFrom(*m_response_message_ptr);
    proto::response_message output_response;
    event sync_event;
    EXPECT_CALL(m_request_callback, Call)
        .Times(1)
        .WillOnce([&](request_result, auto ans) { output_response = *ans;  sync_event.set(); });

    auto promise = make_promise(m_thread_pool.get(), [](request_result r, buffer &&b) {
        return rpc::ftuple(r, std::move(b));
    });

    EXPECT_CALL(*m_connection_ptr, request_async)
        .Times(1)
        .WillOnce([&](auto &&) { return promise.get_future(); });

    promise.resolve(request_result::ok, create_transfer_msg_res(0, response_result::ok, m_response_message_ptr));

    call_method();

    sync_event.wait();
    ASSERT_TRUE(MessageDifferencer::Equals(response_message_copy, output_response));
}
