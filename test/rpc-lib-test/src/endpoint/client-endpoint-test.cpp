#include <rpc-lib/endpoint/client-endpoint.h>
#include <rpc-lib/authenticator/simple-authenticator/simple-authenticator.h>
#include <rpc-lib/pipe/memory-pipe/mem-pipe-env.h>
#include <rpc-lib/pipe/ipipe-endpoint.h>
#include <rpc-lib/transfer-message/transfer-message.h>
#include <rpc-lib/closure-guard/closure-guard.h>

#include <common-lib/synchronization/event/event.h>

#pragma warning(push, 0)
#include "proto/test-messages.pb.h"
#include <google/protobuf/util/message_differencer.h>
#pragma warning(pop)

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin;
using namespace vshalygin::rpc;
using namespace vshalygin::cl;
using namespace testing;
using namespace google::protobuf::util;

namespace {
    class Service
        : public proto::Service
    {
    public:
        Service() {};

        void Method(::google::protobuf::RpcController *controller,
                    const ::proto::request_message *request,
                    ::proto::response_message *response,
                    ::google::protobuf::Closure *done) override
        {
            closure_guard g{ done };
            auto control = to_response_controller(controller);
            m_last_connection_id = control->get_connection_id();
            ++m_method_call_times;
            EXPECT_TRUE(MessageDifferencer::Equals(*request, m_expected_req_message));
            response->CopyFrom(m_res_message);
        }

        void set_expected_request_message(const ::proto::request_message &req)
        {
            m_expected_req_message = req;
        }

        void set_response_message(const ::proto::response_message &res)
        {
            m_res_message = res;
        }

        unsigned get_method_call_times() const
        {
            return m_method_call_times;
        }

        uint64_t get_last_connection_id() const
        {
            return m_last_connection_id;
        }

    private:
        unsigned m_method_call_times = 0;
        uint64_t m_last_connection_id = 2134234;

        ::proto::request_message m_expected_req_message;
        ::proto::response_message m_res_message;
    };
}

class ClientEndpoint
    : public Test
{
protected:
    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(2);
        m_authenticator = std::make_shared<simple_authenticator>();
        m_pipe_env = std::make_shared<mem_pipe_env>(m_thread_pool);
        m_client_service = std::make_shared<Service>();
        m_sut = std::make_unique<client_endpoint<proto::Service_Stub, proto::Service>>(m_thread_pool,
                                                                                       m_authenticator,
                                                                                       m_pipe_env,
                                                                                       m_client_service);
    }

    void TearDown() override
    {
        m_other.reset();
        m_sut.reset();
        m_pipe_env.reset();

        m_thread_pool->stop();
    }

    void create_and_init_other_pipe_endpoint()
    {
        m_pipe_env->create_pipe()
            .get()
            .apply([this](pipe_wait_res, std::shared_ptr<ipipe_endpoint> p) {
                       m_other = p;
                   });

        m_other->read_async().get();
        m_other->write_async({}).get();
    }

protected:
    std::shared_ptr<ipipe_endpoint> m_other;
    std::shared_ptr<thread_pool> m_thread_pool;
    std::shared_ptr<iauthenticator> m_authenticator;
    std::shared_ptr<mem_pipe_env> m_pipe_env;
    std::shared_ptr<Service> m_client_service;
    std::unique_ptr<client_endpoint<proto::Service_Stub, proto::Service>> m_sut;
};

TEST_F(ClientEndpoint, InitiallyIsNotConnected)
{
    ASSERT_FALSE(m_sut->is_connected());
}

TEST_F(ClientEndpoint, Connect)
{
    auto f = m_sut->connect(std::chrono::seconds(10));
    create_and_init_other_pipe_endpoint();

    f.get();
    ASSERT_TRUE(m_sut->is_connected());
}

TEST_F(ClientEndpoint, ConnectTimeout)
{
    auto f = m_sut->connect(std::chrono::milliseconds(1));

    ASSERT_ANY_THROW(f.get());
    ASSERT_FALSE(m_sut->is_connected());
}

TEST_F(ClientEndpoint, MakeRequestFailsIfNoConnection)
{
    proto::request_message request;
    request.set_data(34);

    auto f = m_sut->make_request<proto::request_message, proto::response_message>(&proto::Service_Stub::Method,
                                                                                  request);


    f.get().apply([](request_result r, std::unique_ptr<proto::response_message> &&) {
        ASSERT_EQ(r, request_result::no_connection);
    });
}

TEST_F(ClientEndpoint, MakeRequest)
{
    proto::request_message request;
    request.set_data(34);
    proto::response_message response;
    request.set_data(45);
    auto req_message = create_transfer_msg_req(0, 0, &request);
    auto res_message = create_transfer_msg_res(0, response_result::ok, &response);

    auto f = m_sut->connect(std::chrono::seconds(10));
    create_and_init_other_pipe_endpoint();
    f.get();

    auto f2 = m_sut->make_request<proto::request_message, proto::response_message>(&proto::Service_Stub::Method,
                                                                                   request);


    m_other->read_async().get().apply([&](pipe_op_res, buffer &&b) {
        ASSERT_EQ(req_message, b);
    });
    m_other->write_async(res_message.copy());

    f2.get().apply([&](request_result r, std::unique_ptr<proto::response_message> &&res) {
        ASSERT_EQ(r, request_result::ok);
        ASSERT_TRUE(MessageDifferencer::Equals(*res, response));
    });
}

TEST_F(ClientEndpoint, SetsDisconnectCallback)
{
    event sync_event;
    MockFunction<void()> disconnect_callback;
    EXPECT_CALL(disconnect_callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    auto f = m_sut->connect(std::chrono::seconds(10));
    create_and_init_other_pipe_endpoint();

    f.then([&](rpc::future<void> &f) { f.then(disconnect_callback.AsStdFunction()); }).get();

    m_other->invalidate();
    sync_event.wait();
}

TEST_F(ClientEndpoint, ServiceProcessesReqeust)
{
    proto::request_message request;
    request.set_data(34);
    proto::response_message response;
    request.set_data(45);
    auto req_message = create_transfer_msg_req(0, 0, &request);
    auto res_message = create_transfer_msg_res(0, response_result::ok, &response);
    m_client_service->set_expected_request_message(request);
    m_client_service->set_response_message(response);

    auto f = m_sut->connect(std::chrono::seconds(10));
    create_and_init_other_pipe_endpoint();
    f.get();

    m_other->write_async(req_message.copy());
    m_other->read_async().get().apply([&](pipe_op_res r, buffer &&b) {
        ASSERT_EQ(r, pipe_op_res::success);
        EXPECT_EQ(b, res_message);
    });
    EXPECT_EQ(m_client_service->get_last_connection_id(), 0u);
    EXPECT_EQ(m_client_service->get_method_call_times(), 1u);
}
