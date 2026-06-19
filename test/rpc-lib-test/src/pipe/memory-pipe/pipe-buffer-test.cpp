//#include <rpc-lib/pipe/memory-pipe/mem-buffer.h>
//#include <common-lib/syncronization/event/event.h>
//
//#include <gtest/gtest.h>
//#include <gmock/gmock.h>
//
//using namespace vshalygin::rpc;
//using namespace vshalygin::cl;
//using namespace testing;
//
//namespace {
//    //TODO move to test lib
//    MATCHER_P2(BufferEq, expected, size, "Arrays are equal") {
//        for(size_t i = 0; i < size; ++i) {
//            if(arg[i] != expected[i]) {
//                return false;
//            }
//        }
//        return true;
//    }
//}
//
//TEST(MemBuffer, IsValidAfterCreation)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//
//    mem_buffer sut(pool);
//
//    ASSERT_TRUE(sut.is_valid());
//}
//
//TEST(MemBuffer, IsNotValidAfterInvalidation)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//
//    mem_buffer sut(pool);
//    sut.invalidate();
//
//    ASSERT_FALSE(sut.is_valid());
//}
//
//TEST(MemBuffer, AddMessageInBufferOnWriteOperationIfNoPendingReadHandlers)
//{
//    buffer buf(2);
//    buf[0] = (std::byte)0x1;
//    buf[1] = (std::byte)0x2;
//
//    event sync_event;
//    MockFunction<void(pipe_op_res)> mock1;
//    EXPECT_CALL(mock1, Call)
//        .Times(1)
//        .WillOnce([&]() { sync_event.set(); });
//    auto pool = std::make_shared<thread_pool>(2);
//
//    mem_buffer sut(pool);
//    sut.write_async(buf.copy(), mock1.AsStdFunction());
//    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
//
//    ASSERT_EQ(sut.get_pending_messages_count(), 1);
//}
//
//TEST(MemBuffer, CallWriteHandlerWithTrueParameterIfWritingSucceeded)
//{
//    buffer buf(2);
//    buf[0] = (std::byte)0x1;
//    buf[1] = (std::byte)0x2;
//    event sync_event;
//    MockFunction<void(pipe_op_res)> mock1;
//    EXPECT_CALL(mock1, Call(pipe_op_res::success))
//        .Times(1)
//        .WillOnce([&]() { sync_event.set(); });
//
//    auto pool = std::make_shared<thread_pool>(2);
//    mem_buffer sut(pool);
//    sut.write_async(buf.copy(), mock1.AsStdFunction());
//    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
//}
//
//TEST(MemBuffer, CallPendingReadHandlerAfterWrite)
//{
//    buffer buf(2);
//    buf[0] = (std::byte)0x1;
//    buf[1] = (std::byte)0x2;
//    InSequence s;
//    event sync_event;
//    MockFunction<void(pipe_op_res)> write_mock;
//    MockFunction<void(pipe_op_res, buffer &&)> read_mock;
//    EXPECT_CALL(write_mock, Call)
//        .Times(1);
//    EXPECT_CALL(read_mock, Call(pipe_op_res::success, BufferEq(buf.data(), buf.size())))
//        .Times(1)
//        .WillOnce([&]() { sync_event.set(); });
//
//    auto pool = std::make_shared<thread_pool>(2);
//    mem_buffer sut(pool);
//    sut.read_async(read_mock.AsStdFunction());
//    sut.write_async(buf.copy(), write_mock.AsStdFunction());
//    EXPECT_TRUE(sync_event.wait_for(std::chrono::seconds(10)));
//
//    EXPECT_EQ(sut.get_pending_read_handlers_count(), 0);
//    EXPECT_EQ(sut.get_pending_messages_count(), 0);
//}
//
//TEST(MemBuffer, DoesNotCallWriteFunctorIfIsIsNotPresented)
//{
//    buffer buf(2);
//    buf[0] = (std::byte)0x1;
//    buf[1] = (std::byte)0x2;
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = std::make_unique<mem_buffer>(pool);
//    sut->write_async(buf.copy(), {});
//    sut.reset();
//}
//
//TEST(MemBuffer, IfWriteHandlerThrowsItDoesNotInterruptCallingReadCallbacks)
//{
//    buffer buf(2);
//    buf[0] = (std::byte)0x1;
//    buf[1] = (std::byte)0x2;
//    MockFunction<void(pipe_op_res)> write_mock;
//    MockFunction<void(pipe_op_res, buffer &&)> read_mock;
//    EXPECT_CALL(write_mock, Call)
//        .Times(1)
//        .WillOnce(Throw(std::exception()));
//    EXPECT_CALL(read_mock, Call(_, BufferEq(buf.data(), buf.size())))
//        .Times(1);
//
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = std::make_unique<mem_buffer>(pool);
//    sut->read_async(read_mock.AsStdFunction());
//    sut->write_async(buf.copy(), write_mock.AsStdFunction());
//    sut.reset();
//}
//
//TEST(MemBuffer, CallsReadHandlerInstantlyIfInvalidated)
//{
//    MockFunction<void(pipe_op_res, buffer &&)> read_mock;
//    EXPECT_CALL(read_mock, Call(pipe_op_res::failed, _))
//        .Times(1);
//    auto pool = std::make_shared<thread_pool>(2);
//    mem_buffer sut(pool);
//
//    sut.invalidate();
//    sut.read_async(read_mock.AsStdFunction());
//
//    ASSERT_EQ(sut.get_pending_read_handlers_count(), 0);
//}
//
//TEST(MemBuffer, AddsPendingReadHandlerIfNoPendingMessages)
//{
//    MockFunction<void(pipe_op_res, buffer &&)> read_mock;
//    EXPECT_CALL(read_mock, Call)
//        .Times(0);
//    auto pool = std::make_shared<thread_pool>(2);
//    mem_buffer sut(pool);
//
//    sut.read_async(read_mock.AsStdFunction());
//
//    ASSERT_EQ(sut.get_pending_read_handlers_count(), 1);
//    Mock::VerifyAndClearExpectations(&read_mock);
//}
//
//TEST(MemBuffer, CallsReadHandlerInstantlyIfThereIsAPendingMessage)
//{
//    buffer buf(2);
//    buf[0] = (std::byte)0x1;
//    buf[1] = (std::byte)0x2;
//    MockFunction<void(pipe_op_res, buffer &&)> read_mock;
//    EXPECT_CALL(read_mock, Call(pipe_op_res::success, BufferEq(buf.data(), buf.size())))
//        .Times(1);
//    auto pool = std::make_shared<thread_pool>(2);
//    mem_buffer sut(pool);
//
//    sut.write_async(buf.copy(), {});
//    sut.read_async(read_mock.AsStdFunction());
//
//    ASSERT_EQ(sut.get_pending_read_handlers_count(), 0);
//    ASSERT_EQ(sut.get_pending_messages_count(), 0);
//    Mock::VerifyAndClearExpectations(&read_mock);
//}
//
//TEST(MemBuffer, AllowsEmptyReadHandler)
//{
//    auto pool = std::make_shared<thread_pool>(2);
//    mem_buffer sut(pool);
//
//    sut.invalidate();
//    sut.read_async({});
//}
//
//TEST(MemBuffer, InvalidateMethodsTriggersPendingReadCallbacks)
//{
//    MockFunction<void(pipe_op_res, buffer &&)> read_mock;
//    EXPECT_CALL(read_mock, Call(pipe_op_res::canceled, _))
//        .Times(2);
//    auto pool = std::make_shared<thread_pool>(2);
//    mem_buffer sut(pool);
//    sut.read_async(read_mock.AsStdFunction());
//    sut.read_async(read_mock.AsStdFunction());
//
//    sut.invalidate();
//
//    ASSERT_EQ(sut.get_pending_read_handlers_count(), 0);
//}
//
//TEST(MemBuffer, DestructorTriggersPendingReadCallbacks)
//{
//    MockFunction<void(pipe_op_res, buffer &&)> read_mock;
//    EXPECT_CALL(read_mock, Call(pipe_op_res::canceled, _))
//        .Times(2);
//    auto pool = std::make_shared<thread_pool>(2);
//    auto sut = std::make_unique<mem_buffer>(pool);
//    sut->read_async(read_mock.AsStdFunction());
//    sut->read_async(read_mock.AsStdFunction());
//
//    sut.reset();
//}
