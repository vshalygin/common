#include "common-lib/thread-pool/thread-pool.h"
#include "common-lib/syncronization/event/event.h"

#include "gtest/gtest.h"

using namespace vsh::cl;
using namespace testing;

TEST(ThreadPool, CreatesPoolWithSpecifiedNumberOfThreads)
{
    thread_pool sut(2);

    ASSERT_EQ(sut.get_num(), 2u);
}

TEST(ThreadPool, ExecutesTaskInAnotherThread)
{
    event sync_event;
    thread_pool sut(2);

    auto main_thread_id = std::this_thread::get_id();
    sut.post([&]() {
                 ASSERT_NE(std::this_thread::get_id(), main_thread_id);
                 sync_event.set();
             });

    ASSERT_TRUE(sync_event.wait_for(std::chrono::seconds(2)));
}

TEST(ThreadPool, IsRunningAfterCreation)
{
    thread_pool sut(2);

    ASSERT_FALSE(sut.is_stopped());
}

TEST(ThreadPool, IsNotRunningAfterStop)
{
    thread_pool sut(2);

    sut.stop();

    ASSERT_TRUE(sut.is_stopped());
}
