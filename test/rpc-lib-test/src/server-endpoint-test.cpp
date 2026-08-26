#include <rpc-lib/server-endpoint.h>
#include <rpc-lib/authenticator/simple-authenticator/simple-authenticator.h>
#include <rpc-lib/pipe/memory-pipe/mem-pipe-env.h>
#include <rpc-lib/pipe/memory-pipe/mem-pipe-endpoint.h>
#include <rpc-lib/internal/transfer-message/transfer-message.h>
#include <rpc-lib/closure-guard/closure-guard.h>

#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/synchronization/event.h>

#pragma warning(push, 0)
#include <test-messages.pb.h>
#include <google/protobuf/util/message_differencer.h>
#pragma warning(pop)

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin;
using namespace vshalygin::rpc;
using namespace vshalygin::rpc::internal;
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

class ServerEndpoint
    : public Test
{
protected:
    void SetUp() override
    {
        ON_CALL(m_on_connection_change, Call)
            .WillByDefault([]() {});
        ON_CALL(m_on_state_change, Call)
            .WillByDefault([]() {});

        m_thread_pool = std::make_shared<thread_pool>(2);
        m_authenticator = std::make_shared<simple_authenticator>();
        m_pipe_env = std::make_shared<mem_pipe_env>(m_thread_pool.get());
        m_gservice = std::make_shared<Service>();

        m_sut = std::make_unique<server_endpoint<proto::Service_Stub, proto::Service>>(
            m_on_connection_change.AsStdFunction(),
            m_on_state_change.AsStdFunction(),
            m_thread_pool.get(),
            m_authenticator,
            m_pipe_env,
            m_gservice);
    }

    void TearDown() override
    {
        m_sut.reset();
        m_pipe_env.reset();
        m_thread_pool->stop();
    }

    void create_and_init_other_endpoint()
    {
        m_pipe_env->open_pipe(0).get().apply([&](pipe_wait_res, std::shared_ptr<ipipe_endpoint> pe) {
            m_other = pe;
        });

        m_other->write_async({}).get();
        m_other->read_async().get();
    }

protected:
    MockFunction<void(uint64_t, connection_state)> m_on_connection_change;
    MockFunction<void(server_endpoint_state)> m_on_state_change;

    std::shared_ptr<iauthenticator> m_authenticator;
    std::shared_ptr<mem_pipe_env> m_pipe_env;
    std::shared_ptr<Service> m_gservice;
    std::shared_ptr<thread_pool> m_thread_pool;

    std::unique_ptr<server_endpoint<proto::Service_Stub, proto::Service>> m_sut;

    std::shared_ptr<ipipe_endpoint> m_other;
};

TEST_F(ServerEndpoint, InitiallyNotListening)
{
    ASSERT_FALSE(m_sut->is_listening());
}

TEST_F(ServerEndpoint, IsListeningAfterStartListen)
{
    m_sut->start_listening();

    ASSERT_TRUE(m_sut->is_listening());
}

TEST_F(ServerEndpoint, IsNotListeningAfterStopListen)
{
    m_sut->start_listening();

    m_sut->stop_listening();

    ASSERT_FALSE(m_sut->is_listening());
}

TEST_F(ServerEndpoint, NotifyAboutStartListening)
{
    event sync_event;
    EXPECT_CALL(m_on_state_change, Call(server_endpoint_state::start_listening))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    m_sut->start_listening();
    sync_event.wait();
    Mock::VerifyAndClearExpectations(&m_on_state_change);
}

TEST_F(ServerEndpoint, NotifyAboutStopListening)
{
    event sync_event;
    EXPECT_CALL(m_on_state_change, Call(server_endpoint_state::start_listening))
        .Times(1);
    EXPECT_CALL(m_on_state_change, Call(server_endpoint_state::stop_listening))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    m_sut->start_listening();

    m_sut->stop_listening();

    sync_event.wait();
    Mock::VerifyAndClearExpectations(&m_on_state_change);
}

TEST_F(ServerEndpoint, NotifyAboutStopListeningOnDestruction)
{
    event sync_event;
    EXPECT_CALL(m_on_state_change, Call(server_endpoint_state::start_listening))
        .Times(1);
    EXPECT_CALL(m_on_state_change, Call(server_endpoint_state::stop_listening))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    m_sut->start_listening();

    m_sut.reset();

    sync_event.wait();
    Mock::VerifyAndClearExpectations(&m_on_state_change);
}

TEST_F(ServerEndpoint, CreateNewConnection)
{
    event sync_event;
    EXPECT_CALL(m_on_connection_change, Call(_, connection_state::connected))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    m_sut->start_listening();

    create_and_init_other_endpoint();
    sync_event.wait();
    EXPECT_EQ(m_sut->get_active_connections_count(), 1);
    Mock::VerifyAndClearExpectations(&m_on_connection_change);
}

TEST_F(ServerEndpoint, NotifyAboutConnectionInterrupt)
{
    uint64_t id;
    event sync_event;
    EXPECT_CALL(m_on_connection_change, Call(_, connection_state::connected))
        .Times(1)
        .WillOnce(SaveArg<0>(&id));
    EXPECT_CALL(m_on_connection_change, Call(_, connection_state::disconnected))
        .Times(1)
        .WillOnce([&](auto i, auto) { EXPECT_EQ(i, id); sync_event.set(); });

    m_sut->start_listening();

    create_and_init_other_endpoint();

    m_other->invalidate();

    sync_event.wait();
    EXPECT_EQ(m_sut->get_active_connections_count(), 0);
    Mock::VerifyAndClearExpectations(&m_on_connection_change);
}

TEST_F(ServerEndpoint, NotifyAboutConnectionInterruptOnDestruction)
{
    uint64_t id;
    event sync_event;
    EXPECT_CALL(m_on_connection_change, Call(_, connection_state::connected))
        .Times(1)
        .WillOnce(SaveArg<0>(&id));
    EXPECT_CALL(m_on_connection_change, Call(_, connection_state::disconnected))
        .Times(1)
        .WillOnce([&](auto i, auto) { EXPECT_EQ(i, id); sync_event.set(); });

    m_sut->start_listening();

    create_and_init_other_endpoint();
    while(m_sut->get_active_connections_count() == 0) {}

    m_sut.reset();

    sync_event.wait();
    Mock::VerifyAndClearExpectations(&m_on_connection_change);
}

TEST_F(ServerEndpoint, MakeRequestReturnsNoConnectionCode)
{
    uint64_t id;
    event sync_event;
    EXPECT_CALL(m_on_connection_change, Call(_, connection_state::connected))
        .Times(1)
        .WillOnce([&](auto i, auto) {
            id = i;
            sync_event.set();
        });
    EXPECT_CALL(m_on_connection_change, Call(_, connection_state::disconnected))
        .Times(1);
    m_sut->start_listening();
    create_and_init_other_endpoint();
    sync_event.wait();

    auto ans = m_sut->make_request<proto::request_message, proto::response_message>(
        id + 1000, &proto::Service_Stub::Method, proto::request_message{});

    ans.get().apply([](request_result r, std::unique_ptr<proto::response_message> &&) {
        EXPECT_EQ(r, request_result::no_connection);
    });
}

TEST_F(ServerEndpoint, MakeRequest)
{
    proto::request_message request;
    request.set_data(34);
    proto::response_message response;
    request.set_data(45);
    auto req_message = create_transfer_msg_req(0, 0, &request);
    auto res_message = create_transfer_msg_res(0, response_result::ok, &response);

    uint64_t id;
    event sync_event;
    EXPECT_CALL(m_on_connection_change, Call(_, connection_state::connected))
        .Times(1)
        .WillOnce([&](auto i, auto) { id = i; sync_event.set(); });
    EXPECT_CALL(m_on_connection_change, Call(_, connection_state::disconnected))
        .Times(1);
    m_sut->start_listening();
    create_and_init_other_endpoint();
    while(m_sut->get_active_connections_count() == 0) {}
    sync_event.wait();

    auto ans = m_sut->make_request<proto::request_message, proto::response_message>(
        id, &proto::Service_Stub::Method, request);
    m_other->write_async(res_message.copy());

    ans.get().apply([&](request_result r, std::unique_ptr<proto::response_message> &&res) {
        EXPECT_EQ(r, request_result::ok);
        EXPECT_TRUE(MessageDifferencer::Equals(*res, response));
    });
}

TEST_F(ServerEndpoint, MakeRequestAllWithNoConnections)
{
    auto ans = m_sut->make_request_all<proto::request_message, proto::response_message>(
        &proto::Service_Stub::Method, proto::request_message{});

    ASSERT_TRUE(ans.empty());
}

TEST_F(ServerEndpoint, MakeRequestAll)
{
    proto::request_message request;
    request.set_data(34);
    proto::response_message response;
    request.set_data(45);
    auto req_message = create_transfer_msg_req(0, 0, &request);
    auto res_message = create_transfer_msg_res(0, response_result::ok, &response);

    uint64_t id;
    event sync_event;
    EXPECT_CALL(m_on_connection_change, Call(_, connection_state::connected))
        .Times(1)
        .WillOnce([&](auto i, auto) { id = i; sync_event.set(); });
    EXPECT_CALL(m_on_connection_change, Call(_, connection_state::disconnected))
        .Times(1);
    m_sut->start_listening();
    create_and_init_other_endpoint();
    while(m_sut->get_active_connections_count() == 0) {}
    sync_event.wait();

    auto ans = m_sut->make_request_all<proto::request_message, proto::response_message>(
         &proto::Service_Stub::Method, request);
    m_other->write_async(res_message.copy());

    ASSERT_EQ(ans.size(), 1);
    ASSERT_EQ(ans[0].first, id);
    ans[0].second.get().apply([&](request_result r, std::unique_ptr<proto::response_message> &&res) {
        EXPECT_EQ(r, request_result::ok);
        EXPECT_TRUE(MessageDifferencer::Equals(*res, response));
    });
}

TEST_F(ServerEndpoint, ServiceProcessesRequest)
{
    proto::request_message request;
    request.set_data(34);
    proto::response_message response;
    request.set_data(45);
    auto req_message = create_transfer_msg_req(0, 0, &request);
    auto res_message = create_transfer_msg_res(0, response_result::ok, &response);
    m_gservice->set_expected_request_message(request);
    m_gservice->set_response_message(response);

    uint64_t id;
    event sync_event;
    EXPECT_CALL(m_on_connection_change, Call(_, connection_state::connected))
        .Times(1)
        .WillOnce([&](auto i, auto) { id = i; sync_event.set(); });
    EXPECT_CALL(m_on_connection_change, Call(_, connection_state::disconnected))
        .Times(1);
    m_sut->start_listening();
    create_and_init_other_endpoint();
    while(m_sut->get_active_connections_count() == 0) {}
    sync_event.wait();

    m_other->write_async(req_message.copy());
    m_other->read_async().get().apply([&](pipe_op_res r, buffer &&b) {
        ASSERT_EQ(r, pipe_op_res::success);
        EXPECT_EQ(b, res_message);
    });

    EXPECT_EQ(m_gservice->get_last_connection_id(), id);
    EXPECT_EQ(m_gservice->get_method_call_times(), 1u);
}
