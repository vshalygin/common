#include <common-lib/pipe/pipe-endpoint.h>
#include <common-lib/thread-pool/thread-pool.h>
#include <common-lib/syncronization/event/event.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace vsh::cl;
using namespace testing;

namespace {
    //TODO move to test lib
    MATCHER_P2(ArrayEq, expected, size, "Arrays are equal") {
        for(size_t i = 0; i < size; ++i) {
            if(arg[i] != expected[i]) {
                return false;
            }
        }
        return true;
    }
}

class BufferEndpoint
    : public Test
{
protected:
    void SetUp() override
    {
        m_thread_pool = std::make_shared<thread_pool>(2);
        m_input_buffer = pipe_buffer::create(m_thread_pool);
        m_output_buffer = pipe_buffer::create(m_thread_pool);
        
        m_sut = std::make_unique<pipe_endpoint>(m_input_buffer, m_output_buffer);
    }

protected:
    std::shared_ptr<thread_pool> m_thread_pool;
    std::shared_ptr<pipe_buffer> m_input_buffer;
    std::shared_ptr<pipe_buffer> m_output_buffer;

    std::unique_ptr<pipe_endpoint> m_sut;
    
};

TEST_F(BufferEndpoint, AnswersNotConnectedAfterConstruction)
{
    ASSERT_FALSE(m_sut->is_connected());
}

TEST_F(BufferEndpoint, AnswersNotConnectedIfOnlyInputBufferEnabled)
{
    m_input_buffer->enable();
    ASSERT_FALSE(m_sut->is_connected());
}

TEST_F(BufferEndpoint, AnswersNotConnectedIfOnlyOutputBufferEnabled)
{
    m_output_buffer->enable();
    ASSERT_FALSE(m_sut->is_connected());
}

TEST_F(BufferEndpoint, AnswersConnectedIfBothBuffersEnabled)
{
    m_input_buffer->enable();
    m_output_buffer->enable();
    ASSERT_TRUE(m_sut->is_connected());
}

TEST_F(BufferEndpoint, DisablesBufferOnDestruction)
{
    m_input_buffer->enable();
    m_output_buffer->enable();

    m_sut.reset();

    ASSERT_FALSE(m_input_buffer->is_enabled());
    ASSERT_FALSE(m_output_buffer->is_enabled());
}

TEST_F(BufferEndpoint, WritesMessageToOutputBuffer)
{
    event sync_event;
    buffer buf(1); buf[0] = std::byte(0x16);
    m_input_buffer->enable();
    m_output_buffer->enable();

    m_sut->write_async(buf.copy(), {});
    
    MockFunction<void(pipe_result, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_result::ok, ArrayEq(buf.data(), buf.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });
    m_output_buffer->read_async(read_callback.AsStdFunction());
    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}

TEST_F(BufferEndpoint, ReadsMessageFromInputBuffer)
{
    event sync_event;
    buffer buf(1); buf[0] = std::byte(0x16);
    m_input_buffer->enable();
    m_output_buffer->enable();
    m_input_buffer->write_async(buf.copy(), {});
    MockFunction<void(pipe_result, buffer &&)> read_callback;
    EXPECT_CALL(read_callback, Call(pipe_result::ok, ArrayEq(buf.data(), buf.size())))
        .Times(1)
        .WillOnce([&]() { sync_event.set(); });

    m_sut->read_async(read_callback.AsStdFunction());

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
}