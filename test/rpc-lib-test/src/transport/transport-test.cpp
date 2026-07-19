#include "rpc-lib/internal/transport/transport.h"
#include "rpc-lib/pipe/memory-pipe/mem-pipe-env.h"
#include "rpc-lib/pipe/ipipe-endpoint.h"
#include "common-lib/synchronization/event.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <memory>

using namespace vshalygin::rpc;
using namespace vshalygin::rpc::internal;
using namespace vshalygin::cl;
using namespace testing;

namespace {
    //TODO move to test lib
    MATCHER_P2(BufferEq, expected, size, "Buffers are equal") {
        if(arg.size() != size) {
            return false;
        }
        for(size_t i = 0; i < size; ++i) {
            if(arg[i] != expected[i]) {
                return false;
            }
        }
        return true;
    }
}

class Transport
    : public Test
{
protected:
    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(2);
        auto m_pipe_env = mem_pipe_env(m_thread_pool);
        auto f1 = m_pipe_env.create_pipe()
            .then([&](pipe_wait_res, std::shared_ptr<ipipe_endpoint> p) {
                      m_pipe_endpoint = std::move(p);
                  });
         auto f2 = m_pipe_env.open_pipe()
             .then([&](pipe_wait_res, std::shared_ptr<ipipe_endpoint> p) {
                      m_other_pipe_endpoint = std::move(p);
                  });

        f1.get(); f2.get();
        m_sut = std::make_unique<transport>(m_pipe_endpoint, std::chrono::milliseconds(10000));
    }

    void TearDown() override
    {
        m_sut.reset();
        m_other_pipe_endpoint.reset();
        m_pipe_endpoint.reset();
        m_thread_pool->stop();
    }

protected:
    MockFunction<void()> m_stop_callback;

    std::shared_ptr<thread_pool> m_thread_pool;

    std::shared_ptr<ipipe_endpoint> m_pipe_endpoint;
    std::shared_ptr<ipipe_endpoint> m_other_pipe_endpoint;

    std::unique_ptr<transport> m_sut;
};

TEST_F(Transport, IsRunningIfPipeEndpointIsConnected)
{
    ASSERT_TRUE(m_sut->is_running());
}

TEST_F(Transport, IsNotRunningAfterStop)
{
    m_sut->stop();

    ASSERT_FALSE(m_sut->is_running());
}

TEST_F(Transport, SendAsync)
{
    buffer buf(1); buf[0] = std::byte(34);
    MockFunction<void(pipe_op_res)> write_callback;
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(write_callback, Call(pipe_op_res::success))
        .Times(1);
    EXPECT_CALL(read_callback, Call)
        .Times(1);

    m_sut->send_async(buf.copy())
        .then(write_callback.AsStdFunction())
        .get();
    m_other_pipe_endpoint->read_async()
        .then(read_callback.AsStdFunction())
        .get();
}

TEST_F(Transport, RecvAsync)
{
    buffer buf(1); buf[0] = std::byte(34);
    MockFunction<void(pipe_op_res)> write_callback;
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(write_callback, Call(pipe_op_res::success))
        .Times(1);
    EXPECT_CALL(read_callback, Call(pipe_op_res::success, BufferEq(buf.data(), buf.size())))
        .Times(1);

    m_other_pipe_endpoint->write_async(buf.copy())
        .then(write_callback.AsStdFunction())
        .get();
    m_sut->recv_async()
        .then(read_callback.AsStdFunction())
        .get();
}

TEST_F(Transport, SetsStopCallback)
{
    auto sync_event = std::make_shared<event>();
    EXPECT_CALL(m_stop_callback, Call)
        .Times(1)
        .WillOnce([sync_event]() { sync_event->set(); });

    m_sut->set_stop_callback(m_stop_callback.AsStdFunction());
    m_sut->stop();

    sync_event->wait();
    Mock::VerifyAndClearExpectations(&m_stop_callback);
}

TEST_F(Transport, InvalidatesPipeEndOnDestruction)
{
    EXPECT_TRUE(m_pipe_endpoint->is_connected());

    m_sut.reset();

    EXPECT_FALSE(m_pipe_endpoint->is_connected());
}

TEST_F(Transport, SendTimeout)
{
    auto sync_event = std::make_shared<event>();
    for(unsigned i = 0; i < m_thread_pool->get_num(); ++i) {
        m_thread_pool->post([sync_event]() { sync_event->wait(); });
    }
    MockFunction<void(pipe_op_res)> write_callback;
    EXPECT_CALL(write_callback, Call(pipe_op_res::timeout))
        .Times(1);

    transport sut(m_pipe_endpoint, std::chrono::milliseconds(0));
    auto f = sut.send_async({})
        .then(write_callback.AsStdFunction());

    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    sync_event->set();
    f.get();
}
