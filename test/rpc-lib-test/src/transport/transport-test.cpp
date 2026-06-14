#include <rpc-lib/transport/transport.h>
#include <rpc-lib/pipe/ipipe.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vshalygin::rpc;
using namespace vshalygin::cl;

using namespace testing;

namespace {
    //TODO move to test lib
    MATCHER_P2(BufferEq, expected, size, "Buffers are equal") {
        for(size_t i = 0; i < size; ++i) {
            if(arg[i] != expected[i]) {
                return false;
            }
        }
        return true;
    }

    class pipe_mock
        : public ipipe
    {
    public:
        MOCK_METHOD(bool, is_connected, (), (const, override));

        MOCK_METHOD(bool, wait_connect_for, (const std::chrono::microseconds &), (const, override));
        MOCK_METHOD(bool, wait_connect, (), (const, override));

        MOCK_METHOD(bool, write_async, (buffer &&, std::function<void(pipe_op_res)> &&), (override));
        MOCK_METHOD(bool, read_async, (std::function<void(pipe_op_res, buffer &&)> &&), (override));

        MOCK_METHOD(bool, try_to_write_for, (buffer &&msg, const std::chrono::microseconds &), (override));
        MOCK_METHOD(std::optional<buffer>, try_to_read_for, (const std::chrono::microseconds &), (override));

        MOCK_METHOD(void, invalidate, (), (override));
    };

    using pipe_nice_mock = NiceMock<pipe_mock>;
}

class Transport
    : public Test
{
protected:
    void SetUp() override
    {
        m_pipe = std::make_shared<pipe_nice_mock>();
        ON_CALL(*m_pipe, is_connected)
            .WillByDefault(Return(true));
        ON_CALL(*m_pipe, wait_connect_for)
            .WillByDefault(Return(true));
        m_sut = std::make_unique<transport>(m_pipe);
    }

protected:
    std::unique_ptr<itransport> m_sut;
    std::shared_ptr<pipe_nice_mock> m_pipe;
};

TEST_F(Transport, SendAsyncMessage)
{
    buffer message(1); message[0] = std::byte(0x1);
    EXPECT_CALL(*m_pipe, write_async(BufferEq(message.data(), message.size()), _))
        .Times(1)
        .WillOnce(Return(true));

    m_sut->send_async(message.copy(), {});
}

TEST_F(Transport, CallsSendErrorCallbackOnError)
{
    MockFunction<void()> error_handler;
    EXPECT_CALL(error_handler, Call)
        .Times(1);

    EXPECT_CALL(*m_pipe, write_async)
        .Times(1)
        .WillOnce([](buffer &&, std::function<void(pipe_op_res)> &&eh) {
                      eh(pipe_op_res::failed);
                      return true;
                  });

    m_sut->send_async({}, error_handler.AsStdFunction());
}

TEST_F(Transport, DoesNotCallSendErrorCallbackOnSuccessOrCancel)
{
    MockFunction<void()> error_handler;
    EXPECT_CALL(error_handler, Call)
        .Times(0);

    EXPECT_CALL(*m_pipe, write_async)
        .Times(1)
        .WillOnce([](buffer &&, std::function<void(pipe_op_res)> &&eh) {
                      eh(pipe_op_res::success);
                      eh(pipe_op_res::canceled);
                      return true;
                  });

    m_sut->send_async({}, error_handler.AsStdFunction());
}

TEST_F(Transport, ThrowsExceptionIfWriteAsyncReturnsFalse)
{
    EXPECT_CALL(*m_pipe, write_async)
        .Times(1)
        .WillOnce(Return(false));

    ASSERT_ANY_THROW(m_sut->send_async({}, {}));
}

TEST_F(Transport, RecvAsync)
{
    buffer message(1); message[0] = std::byte(0x1);
    EXPECT_CALL(*m_pipe, read_async)
        .Times(1)
        .WillOnce([&](std::function<void(pipe_op_res res, buffer &&msg)> &&eh) {
                       eh(pipe_op_res::success, message.copy());
                       return true;
                   });

    MockFunction<void(bool, buffer &&)> callback;
    EXPECT_CALL(callback, Call(true, BufferEq(message.data(), message.size())))
        .Times(1);

    m_sut->recv_async(callback.AsStdFunction());
}

TEST_F(Transport, RecvAsyncCallsHandlerWithFalseParameterOnFail)
{
    EXPECT_CALL(*m_pipe, read_async)
        .Times(1)
        .WillOnce([&](std::function<void(pipe_op_res res, buffer &&)> &&eh) {
                      eh(pipe_op_res::failed, {});
                      eh(pipe_op_res::canceled, {});
                      return true;
                  });

    MockFunction<void(bool, buffer &&)> callback;
    EXPECT_CALL(callback, Call(false, _))
        .Times(2);

    m_sut->recv_async(callback.AsStdFunction());
}

TEST_F(Transport, StopIfRecvAsyncReturnsFalse)
{
    EXPECT_CALL(*m_pipe, read_async)
        .Times(1)
        .WillOnce(Return(false));
    NiceMock<MockFunction<void()>> stop_callback;
    NiceMock<MockFunction<void(bool, buffer &&)>> callback;
    m_sut->start({}, stop_callback.AsStdFunction());

    EXPECT_CALL(*m_pipe, invalidate)
        .Times(1);
    EXPECT_CALL(stop_callback, Call)
        .Times(1);

    m_sut->recv_async(callback.AsStdFunction());
}

TEST_F(Transport, ThrowsExceptionOnAttemptToStartTwice)
{
    ASSERT_NO_THROW(m_sut->start({}, {}));
    ASSERT_ANY_THROW(m_sut->start({}, {}));
}

TEST_F(Transport, ThrowsExceptionOnAttemptToStartAgain)
{
    m_sut->start({}, {});
    m_sut->stop();

    ASSERT_ANY_THROW(m_sut->start({}, {}));
}

TEST_F(Transport, ThrowsIfFailedToWaitConnectionOnStart)
{
    EXPECT_CALL(*m_pipe, wait_connect_for)
        .WillOnce(Return(false));

    NiceMock<MockFunction<void()>> start_callback;
    NiceMock<MockFunction<void()>> stop_callback;

    ASSERT_ANY_THROW(m_sut->start(start_callback.AsStdFunction(), stop_callback.AsStdFunction()));
}

TEST_F(Transport, HasRunningStatusAfterStart)
{
    NiceMock<MockFunction<void()>> start_callback;
    NiceMock<MockFunction<void()>> stop_callback;

    m_sut->start({}, {});

    ASSERT_TRUE(m_sut->is_running());
}

TEST_F(Transport, CallsStartCallbackAfterStart)
{
    NiceMock<MockFunction<void()>> start_callback;
    EXPECT_CALL(start_callback, Call)
        .Times(1);

    m_sut->start(start_callback.AsStdFunction(), {});
}

TEST_F(Transport, CatchesExceptionsInStartCallback)
{
    NiceMock<MockFunction<void()>> start_callback;
    EXPECT_CALL(start_callback, Call)
        .Times(1)
        .WillOnce(Throw(std::runtime_error("")));

    ASSERT_NO_THROW(m_sut->start(start_callback.AsStdFunction(), {}));

    m_sut.reset();
}

TEST_F(Transport, DoesNotStartAgainAfterStop)
{
    m_sut->start({}, {});
    m_sut->stop();

    ASSERT_ANY_THROW(m_sut->start({}, {}));
}

TEST_F(Transport, InvalidatesPipeAfterStop)
{
    m_sut->start({}, {});
    EXPECT_CALL(*m_pipe, invalidate)
        .Times(1);

    m_sut->stop();
    Mock::VerifyAndClearExpectations(m_pipe.get());
}

TEST_F(Transport, CallsStopCallbackOnStop)
{
    NiceMock<MockFunction<void()>> stop_callback;
    m_sut->start({}, stop_callback.AsStdFunction());

    EXPECT_CALL(stop_callback, Call)
        .Times(1);

    m_sut->stop();
    Mock::VerifyAndClearExpectations(&stop_callback);
}

TEST_F(Transport, DoesNotInvalidatePipeIfItWasNotStarted)
{
    EXPECT_CALL(*m_pipe, invalidate)
        .Times(0);

    m_sut->stop();
    Mock::VerifyAndClearExpectations(m_pipe.get());
}

TEST_F(Transport, DoesNotInvalidatePipeAndCallStopCallbackTwiceIfStopWasCalledTwoTimes)
{
    NiceMock<MockFunction<void()>> stop_callback;
    m_sut->start({}, stop_callback.AsStdFunction());
    EXPECT_CALL(*m_pipe, invalidate)
        .Times(1);
    EXPECT_CALL(stop_callback, Call)
        .Times(1);

    m_sut->stop();
    m_sut->stop();
    Mock::VerifyAndClearExpectations(m_pipe.get());
}

TEST_F(Transport, CatchesExceptionsInStopCallback)
{
    NiceMock<MockFunction<void()>> stop_callback;
    EXPECT_CALL(stop_callback, Call)
        .Times(1)
        .WillOnce(Throw(std::runtime_error("")));
    m_sut->start({}, stop_callback.AsStdFunction());

    ASSERT_NO_THROW(m_sut->stop());
}

TEST_F(Transport, IsNotRunningAfterCreation)
{
    ASSERT_FALSE(m_sut->is_running());
}

TEST_F(Transport, IsRunningAfterStart)
{
    m_sut->start({}, {});

    ASSERT_TRUE(m_sut->is_running());
}


TEST_F(Transport, IsNotRunningAfterStop)
{
    m_sut->start({}, {});
    m_sut->stop();

    ASSERT_FALSE(m_sut->is_running());
}

TEST_F(Transport, InvalidatesPipeAndCallsStopCallbackOnDesctructionIfWasStarted)
{
    NiceMock<MockFunction<void()>> stop_callback;
    m_sut->start({}, stop_callback.AsStdFunction());
    EXPECT_CALL(*m_pipe, invalidate)
        .Times(1);
    EXPECT_CALL(stop_callback, Call)
        .Times(1);

    m_sut.reset();
    Mock::VerifyAndClearExpectations(m_pipe.get());
    Mock::VerifyAndClearExpectations(&stop_callback);
}
