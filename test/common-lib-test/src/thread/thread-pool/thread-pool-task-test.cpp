#include <common-lib/thread/thread-pool/thread-pool-task.h>
#include <common-lib/synchronization/event.h>
#include <common-lib/thread/thread-pool/thread-pool.h>

#include <gtest/gtest.h>
#include <atomic>

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

        inline static std::atomic<unsigned> copy_num = 0;
        inline static std::atomic<unsigned> copy_assign_num = 0;
        inline static std::atomic<unsigned> move_num = 0;
        inline static std::atomic<unsigned> move_assign_num = 0;

        inline static void clear()
        {
            copy_num = 0;
            copy_assign_num = 0;
            move_num = 0;
            move_assign_num = 0;
        }
    };

    class simple_functor
    {
    public:
        void operator()() {}
    };

    void simple_function() {}
}

TEST(ThreadPoolTask, Init)
{
    thread_pool pool(2);

    thread_pool_task<void()> sut1;
    ASSERT_FALSE(sut1);

    thread_pool_task<void()> sut2(&pool, []() {});
    ASSERT_TRUE(sut2);
}

TEST(ThreadPoolTask, ThrowsOnAttemptToExecuteEmptyTask)
{
    thread_pool_task<void()> sut1;
    ASSERT_ANY_THROW(sut1.exec());
}

TEST(ThreadPoolTask, ThrowsOnAttemptToExecuteTaskTwice)
{
    thread_pool pool(2);

    thread_pool_task sut(&pool, []() {});
    sut.exec();
    ASSERT_ANY_THROW(sut.exec());
}

TEST(ThreadPoolTask, LambaWithCapturedReference)
{
    thread_pool pool(2);
    event sync_event;

    int i = 0;
    thread_pool_task sut(&pool, [&]() { i = 1; sync_event.set(); });

    sut.exec();

    sync_event.wait();
    ASSERT_TRUE(i == 1);
}

TEST(ThreadPoolTask, LambaWithParameter)
{
    thread_pool pool(2);
    event sync_event;
    int i = 0;
    thread_pool_task sut(&pool, [&](int v) { i = v; sync_event.set(); });

    sut.exec(2);

    sync_event.wait();
    ASSERT_TRUE(i == 2);
}

TEST(ThreadPoolTask, LambaWithMoveOnlyParameter)
{
    thread_pool pool(2);
    event sync_event;
    int i = 0;
    thread_pool_task sut(
        &pool,
        [&](std::unique_ptr<int> v) { i = *v; sync_event.set(); });

    sut.exec(std::make_unique<int>(2));

    sync_event.wait();
    ASSERT_TRUE(i == 2);
}

TEST(ThreadPoolTask, LambaWithRValueReferenceMoveOnlyParameter)
{
    thread_pool pool(2);
    event sync_event;
    int i = 0;
    thread_pool_task sut(
        &pool,
        [&](std::unique_ptr<int> &&v) { i = *v; sync_event.set(); });

    auto arg = std::make_unique<int>(2);
    sut.exec(std::move(arg));

    sync_event.wait();
    ASSERT_TRUE(i == 2);
}

TEST(ThreadPoolTask, DoesNotCopyInInnerPresentation)
{
    thread_pool pool(2);
    event sync_event;
    counter::clear();
    counter cc;
    thread_pool_task sut(&pool, [cc = std::move(cc), &sync_event]() { sync_event.set(); });

    sut.exec();

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
    thread_pool_task sut(
        &pool,
        [&sync_event](counter &&c) { auto t = std::move(c); t; sync_event.set(); });

    sut.exec(std::move(cc));

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
    thread_pool_task sut(
        &pool,
        [&](const counter &c) { auto t = c; t; sync_event.set(); });

    sut.exec(std::move(cc));

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
    thread_pool_task sut(
        &pool,
        [&](counter c) { auto t = c; t; sync_event.set(); });

    sut.exec(std::move(cc));

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
    thread_pool_task sut(&pool, lambda);

    sut.exec();

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
    thread_pool_task sut(&pool, lambda);

    lambda = {};
    sut.exec();

    sync_event.wait();
}


TEST(ThreadPoolTask, IsMovable)
{
    thread_pool pool(2);
    counter::clear();
    counter cc;
    thread_pool_task sut(&pool, [cc]() {});
    thread_pool_task sut2(std::move(sut));


    ASSERT_EQ(counter::copy_num, 1u);
    ASSERT_EQ(counter::copy_assign_num, 0u);
    ASSERT_EQ(counter::move_num, 1u);
    ASSERT_EQ(counter::move_assign_num, 0u);
}

TEST(ThreadPoolTask, IsMoveAssignable)
{
    thread_pool pool(2);
    counter::clear();
    counter cc;
    thread_pool_task sut(&pool, [cc]() {});
    thread_pool_task other(&pool, []() {});

    sut = std::move(sut);
    other = std::move(sut);

    ASSERT_EQ(counter::copy_num, 1u);
    ASSERT_EQ(counter::copy_assign_num, 0u);
    ASSERT_EQ(counter::move_num, 1u);
    ASSERT_EQ(counter::move_assign_num, 0u);
}

TEST(ThreadPoolTask, CorrectCopyEmptyTask)
{
    thread_pool pool(2);
    thread_pool_task sut(&pool, []() {});
    thread_pool_task sut2(std::move(sut));

    thread_pool_task sut3(std::move(sut));

    EXPECT_FALSE(sut);
    EXPECT_FALSE(sut3);
}

TEST(ThreadPoolTask, IsBoolConvertible)
{
    thread_pool pool(2);
    int i;
    thread_pool_task<void()> sut1;
    thread_pool_task sut2(&pool, []() {});
    thread_pool_task sut3(&pool, [&i]() {});
    thread_pool_task sut4(&pool, simple_functor{});
    thread_pool_task sut5(&pool, &simple_function);
    thread_pool_task sut6(&pool, std::function<void()>([]() {}));
    thread_pool_task sut7(&pool, std::function<void()>([&i]() {}));
    thread_pool_task sut8(&pool, std::function<void()>(simple_functor{}));
    thread_pool_task sut9(&pool, std::function<void()>{&simple_function});

    EXPECT_FALSE(sut1);
    EXPECT_TRUE(sut2);
    EXPECT_TRUE(sut3);
    EXPECT_TRUE(sut4);
    EXPECT_TRUE(sut5);
    EXPECT_TRUE(sut6);
    EXPECT_TRUE(sut7);
    EXPECT_TRUE(sut8);
    EXPECT_TRUE(sut9);
}

TEST(ThreadPoolTask, AnyCallableObjectMayBeCalled)
{
    thread_pool pool(1);
    int i;
    thread_pool_task sut1(&pool, []() {});
    thread_pool_task sut2(&pool, [&i]() {});
    thread_pool_task sut3(&pool, simple_functor{});
    thread_pool_task sut4(&pool, &simple_function);
    thread_pool_task sut5(&pool, std::function<void()>([]() {}));
    thread_pool_task sut6(&pool, std::function<void()>([&i]() {}));
    thread_pool_task sut7(&pool, std::function<void()>(simple_functor{}));
    thread_pool_task sut8(&pool, std::function<void()>{&simple_function});

    sut1.exec();
    sut2.exec();
    sut3.exec();
    sut4.exec();
    sut5.exec();
    sut6.exec();
    sut7.exec();
    sut8.exec();

    pool.stop();
}

TEST(ThreadPoolTask, ParameterMayBeTypeWithAnyQualifiers)
{
    thread_pool pool(1);
    thread_pool_task sut1(&pool, [](int) {});
    //thread_pool_task sut2(&pool, [](int &) {});
    thread_pool_task sut3(&pool, [](int &&) {});
    thread_pool_task sut4(&pool, [](const int) {});
    thread_pool_task sut5(&pool, [](const int &) {});
    thread_pool_task sut6(&pool, [](const int &&) {});
    thread_pool_task sut7(&pool, [](volatile int) {});
    //thread_pool_task sut8(&pool, [](volatile int &) {});
    thread_pool_task sut9(&pool, [](volatile int &&) {});
    thread_pool_task sut10(&pool, [](const volatile int) {});
    //thread_pool_task sut11(&pool, [](const volatile int &) {});
    thread_pool_task sut12(&pool, [](const volatile int &&) {});

    int i = 0;
    volatile int ii = 0;

    sut1.exec(i);
    //sut2.exec(i);
    sut3.exec(std::move(i));
    sut4.exec(i);
    sut5.exec(i);
    sut6.exec(std::move(i));
    sut7.exec(ii);
    //sut8.exec(ii);
    sut9.exec(std::move(ii));
    sut10.exec(ii);
    //sut11.exec(ii);
    sut12.exec(std::move(ii));
}
