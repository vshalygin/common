#include <common-lib/thread-pool/thread-pool-task.h>
#include <common-lib/syncronization/event/event.h>
#include <common-lib/thread-pool/thread-pool.h>

#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

namespace {
    class counter
    {
    public:
        counter() = default;

        counter(const counter &)
        {
            ++copy_num;
        }

        counter &operator=(const counter &)
        {
            ++copy_assign_num;
        }

        counter(counter &&)
        {
            ++move_num;
        }

        counter &operator=(counter &&)
        {
            ++move_assign_num;
        }

        inline static unsigned copy_num = 0;
        inline static unsigned copy_assign_num = 0;
        inline static unsigned move_num = 0;
        inline static unsigned move_assign_num = 0;

        inline static void clear()
        {
            copy_num = 0;
            copy_assign_num = 0;
            move_num = 0;
            move_assign_num = 0;
        }
    };
}

TEST(ThreadPoolTask, Init)
{
    thread_pool_task<void()> sut([]() {});
    sut;
}

TEST(ThreadPoolTask, LambaWithCapturedReference)
{
    thread_pool pool(2);
    event sync_event;

    int i = 0;
    thread_pool_task<void()> sut([&]() { i = 1; sync_event.set(); });

    pool.post(sut);

    sync_event.wait();
    ASSERT_TRUE(i == 1);
}

TEST(ThreadPoolTask, LambaWithParameter)
{
    thread_pool pool(2);
    event sync_event;
    int i = 0;
    thread_pool_task<void(int)> sut([&](int v) { i = v; sync_event.set(); });

    pool.post(sut, 2);

    sync_event.wait();
    ASSERT_TRUE(i == 2);
}

TEST(ThreadPoolTask, LambaWithMoveOnlyParameter)
{
    thread_pool pool(2);
    event sync_event;
    int i = 0;
    thread_pool_task<void(std::unique_ptr<int>)> sut(
        [&](std::unique_ptr<int> v) { i = *v; sync_event.set(); });

    pool.post(sut, std::make_unique<int>(2));

    sync_event.wait();
    ASSERT_TRUE(i == 2);
}

TEST(ThreadPoolTask, LambaWithRValueReferenceMoveOnlyParameter)
{
    thread_pool pool(2);
    event sync_event;
    int i = 0;
    thread_pool_task<void(std::unique_ptr<int> &&)> sut(
        [&](std::unique_ptr<int> &&v) { i = *v; sync_event.set(); });

    auto arg = std::make_unique<int>(2);
    pool.post(sut, std::move(arg));

    sync_event.wait();
    ASSERT_TRUE(i == 2);
}

TEST(ThreadPoolTask, DoesNotCopyInInnerPresentation)
{
    thread_pool pool(2);
    event sync_event;
    counter::clear();
    counter cc;
    thread_pool_task<void()> sut([cc = std::move(cc), &sync_event]() { sync_event.set(); });

    pool.post(std::move(sut));

    sync_event.wait();
    ASSERT_TRUE(counter::copy_num == 0);
    ASSERT_TRUE(counter::copy_assign_num == 0);
    ASSERT_TRUE(counter::move_num == 2);
    ASSERT_TRUE(counter::move_assign_num == 0);
}

TEST(ThreadPoolTask, DoesNotCopyParameterIfItIsRValueRef)
{
    thread_pool pool(2);
    event sync_event;
    counter::clear();
    counter cc;
    thread_pool_task<void(counter &&)> sut(
        [&sync_event](counter &&c) { auto t = std::move(c); t; sync_event.set(); });

    pool.post(std::move(sut), std::move(cc));

    sync_event.wait();
    EXPECT_EQ(counter::copy_num, 0u);
    EXPECT_EQ(counter::copy_assign_num, 0u);
    EXPECT_TRUE(counter::move_num > 0);
    EXPECT_EQ(counter::move_assign_num, 0u);
}

TEST(ThreadPoolTask, DoesNotCopyParameterMoreThanNeededIfItIsLValueRef)
{
    thread_pool pool(2);
    event sync_event;
    counter::clear();
    counter cc;
    thread_pool_task<void(const counter &)> sut(
        [&](const counter &c) { auto t = c; t; sync_event.set(); });

    pool.post(sut, std::move(cc));

    sync_event.wait();
    EXPECT_EQ(counter::copy_num, 1u);
    EXPECT_EQ(counter::copy_assign_num, 0u);
    EXPECT_TRUE(counter::move_num > 0);
    EXPECT_EQ(counter::move_assign_num, 0u);
}

TEST(ThreadPoolTask, DoesNotCopyParameterMoreThanNeededIfItIsNotRef)
{
    thread_pool pool(2);
    event sync_event;
    counter::clear();
    counter cc;
    thread_pool_task<void(counter)> sut(
        [&](counter c) { auto t = c; t; sync_event.set(); });

    pool.post(sut, std::move(cc));

    sync_event.wait();
    EXPECT_EQ(counter::copy_num, 1u);
    EXPECT_EQ(counter::copy_assign_num, 0u);
    EXPECT_TRUE(counter::move_num > 0);
    EXPECT_EQ(counter::move_assign_num, 0u);
}

TEST(ThreadPoolTask, CopyFunctorIfHaveTo)
{
    thread_pool pool(2);
    event sync_event;
    counter::clear();
    counter cc;
    auto lambda = [cc, &sync_event]() { sync_event.set(); };
    thread_pool_task<void()> sut(lambda);

    pool.post(sut);

    sync_event.wait();
    EXPECT_TRUE(counter::copy_num > 0);
    EXPECT_EQ(counter::copy_assign_num, 0u);
    EXPECT_EQ(counter::move_num, 0u);
    EXPECT_EQ(counter::move_assign_num, 0u);
}

TEST(ThreadPoolTask, ExecutesAfterInitialFunctorDestroyes)
{
    thread_pool pool(2);
    event sync_event;
    counter::clear();
    counter cc;
    std::function lambda = [cc, &sync_event]() { sync_event.set(); };
    thread_pool_task<void()> sut(lambda);

    lambda = {};
    pool.post(sut);

    sync_event.wait();
}

TEST(ThreadPoolTask, IsCopyable)
{
    counter::clear();
    counter cc;
    thread_pool_task<void()> sut([cc]() {});
    thread_pool_task<void()> copy(sut);

    ASSERT_EQ(counter::copy_num, 2u);
    ASSERT_EQ(counter::copy_assign_num, 0u);
    ASSERT_EQ(counter::move_num, 1u);
    ASSERT_EQ(counter::move_assign_num, 0u);
}

TEST(ThreadPoolTask, IsMovable)
{
    counter::clear();
    counter cc;
    thread_pool_task<void()> sut([cc]() {});
    thread_pool_task<void()> sut2(std::move(sut));


    ASSERT_EQ(counter::copy_num, 1u);
    ASSERT_EQ(counter::copy_assign_num, 0u);
    ASSERT_EQ(counter::move_num, 1u);
    ASSERT_EQ(counter::move_assign_num, 0u);
}

TEST(ThreadPoolTask, IsCopyAssignable)
{
    counter::clear();
    counter cc;
    thread_pool_task<void()> sut([cc]() {});
    thread_pool_task<void()> other([](){});

    sut = sut;
    other = sut;

    ASSERT_EQ(counter::copy_num, 2u);
    ASSERT_EQ(counter::copy_assign_num, 0u);
    ASSERT_EQ(counter::move_num, 1u);
    ASSERT_EQ(counter::move_assign_num, 0u);
}

TEST(ThreadPoolTask, IsMoveAssignable)
{
    counter::clear();
    counter cc;
    thread_pool_task<void()> sut([cc]() {});
    thread_pool_task<void()> other([]() {});

    sut = std::move(sut);
    other = std::move(sut);

    ASSERT_EQ(counter::copy_num, 1u);
    ASSERT_EQ(counter::copy_assign_num, 0u);
    ASSERT_EQ(counter::move_num, 1u);
    ASSERT_EQ(counter::move_assign_num, 0u);
}

TEST(ThreadPoolTask, CorrectCopyEmptyTask)
{
    thread_pool_task<void()> sut([]() {});
    thread_pool_task<void()> sut2(std::move(sut));

    thread_pool_task<void()> sut3(std::move(sut)); sut3;
}

TEST(ThreadPoolTask, CorrectCopyAssignEmptyTask)
{
    thread_pool_task<void()> sut([]() {});
    thread_pool_task<void()> sut2(std::move(sut));
    thread_pool_task<void()> sut3([]() {});

    sut3 = sut;
}
