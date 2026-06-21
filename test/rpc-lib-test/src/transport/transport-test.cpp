#include "rpc-lib/transport/transport.h"
#include "rpc-lib/pipe/memory-pipe/mem-pipe-env.h"
#include "rpc-lib/pipe/ipipe-endpoint.h"
#include "common-lib/syncronization/event/event.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::rpc;
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
        Sequence s;
        EXPECT_CALL(m_start_callback, Call)
            .Times(1)
            .WillOnce([]() {});
        EXPECT_CALL(m_stop_callback, Call)
            .Times(1)
            .WillOnce([]() {});

        m_thread_pool = std::make_shared<thread_pool>(2);
        m_pipe_env = std::make_unique<mem_pipe_env>(m_thread_pool);
        m_pipe_endpoint = m_pipe_env->create_pipe();
        m_other_pipe_endpoint = m_pipe_env->open_pipe();

        m_sut = std::make_unique<transport>(m_thread_pool,
                                            m_pipe_endpoint,
                                            m_stop_callback.AsStdFunction(),
                                            m_stop_callback.AsStdFunction());
    }

protected:
    MockFunction<void()> m_start_callback;
    MockFunction<void()> m_stop_callback;

    std::shared_ptr<thread_pool> m_thread_pool;

    std::unique_ptr<ipipe_env> m_pipe_env;
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
    event sync_event;
    buffer buf(1); buf[0] = std::byte(34);
    MockFunction<void(pipe_op_res)> write_callback;
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(write_callback, Call(pipe_op_res::success))
        .Times(1);
    EXPECT_CALL(read_callback, Call)
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    m_sut->send_async(buf.copy(), write_callback.AsStdFunction());
    m_other_pipe_endpoint->read_async(read_callback.AsStdFunction());

    sync_event.wait();
}

TEST_F(Transport, RecvAsync)
{
    event sync_event;
    buffer buf(1); buf[0] = std::byte(34);
    MockFunction<void(pipe_op_res)> write_callback;
    MockFunction<void(pipe_op_res, buffer &&)> read_callback;
    EXPECT_CALL(write_callback, Call(pipe_op_res::success))
        .Times(1);
    EXPECT_CALL(read_callback, Call(pipe_op_res::success, BufferEq(buf.data(), buf.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    m_other_pipe_endpoint->write_async(buf.copy(), write_callback.AsStdFunction());
    m_sut->recv_async(read_callback.AsStdFunction());


    sync_event.wait();
}
