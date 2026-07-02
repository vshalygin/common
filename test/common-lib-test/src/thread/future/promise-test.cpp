#include <common-lib/thread/future/future.h>
#include <common-lib/thread/thread-pool/thread-pool.h>
#include <common-lib/synchronization/event/event.h>

#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

//promise is move only
static_assert(!std::is_copy_constructible_v<promise<thread_pool, int>>);
static_assert(!std::is_copy_assignable_v<promise<thread_pool, int>>);
static_assert(std::is_move_constructible_v<promise<thread_pool, int>>);
static_assert(std::is_move_assignable_v<promise<thread_pool, int>>);

namespace {
    class test_type
    {
    public:
        bool was_moved = false;

        test_type() = default;

        test_type(const test_type &)
        {
            ++copy_num;
        }

        test_type &operator=(const test_type &)
        {
            ++copy_assign_num;
            return *this;
        }

        test_type(test_type &&other)
        {
            ++move_num;
            other.was_moved = true;
        }

        test_type &operator=(test_type &&other)
        {
            ++move_assign_num;
            other.was_moved = true;
            return *this;
        }

        void do_something() const {}

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
}

class Promise
    : public Test
{
protected:
    void SetUp() override
    {
        test_type::clear();
    }

protected:
    thread_pool m_thread_pool{ 2 };
};

TEST_F(Promise, IsValidAfterCreation)
{
    auto sut = make_promise(&m_thread_pool, []() {});

    ASSERT_TRUE(sut.is_valid());
}

TEST_F(Promise, IsNotValidAfterMove)
{
    auto sut = make_promise(&m_thread_pool, []() {});
    auto sut2(std::move(sut));

    ASSERT_TRUE(sut2.is_valid());
    ASSERT_FALSE(sut.is_valid());
}

TEST_F(Promise, IsNotValidAfterMoveAssign)
{
    auto sut = make_promise(&m_thread_pool, []() {});
    auto sut2 = make_promise(&m_thread_pool, []() {});
    sut2 = std::move(sut);

    ASSERT_TRUE(sut2.is_valid());
    ASSERT_FALSE(sut.is_valid());
}

TEST_F(Promise, MoveActuallyDoesMove)
{
    event sync_event;
    int i = 0;
    auto sut = make_promise(&m_thread_pool, [&]() { i = 1; sync_event.set(); });
    auto sut2(std::move(sut));
    sut2.resolve();
    
    sync_event.wait();
    ASSERT_EQ(i, 1);
}

TEST_F(Promise, MoveAssignActuallyDoesMove)
{
    event sync_event;
    int i = 0;
    auto sut = make_promise(&m_thread_pool, [&]() { i = 1; sync_event.set(); });
    auto sut2 = make_promise(&m_thread_pool, []() {});
    sut2 = std::move(sut);

    sut2.resolve();

    sync_event.wait();
    ASSERT_EQ(i, 1);
}

TEST_F(Promise, GetFutureReturnsValidFuture)
{
    auto sut = make_promise(&m_thread_pool, []() {});
    
    auto f = sut.get_future();
    
    ASSERT_TRUE(f.is_valid());
}

TEST_F(Promise, ThrowsExceptionOnAttempltToCallGetFutureTwice)
{
    auto sut = make_promise(&m_thread_pool, []() {});

    sut.get_future();
    ASSERT_ANY_THROW(sut.get_future());
}

TEST_F(Promise, ResolveFunctionStartesFunctionExecutionInAnotherThread)
{
    const auto master_thread_id = std::this_thread::get_id();
    event sync_event;

    auto sut = make_promise(&m_thread_pool, [&]() {
        ASSERT_NE(master_thread_id, std::this_thread::get_id());
        sync_event.set();
    });
    sut.resolve();


    sync_event.wait();
}

TEST_F(Promise, ThrowsExceptionOnAttemptToCallResolveTwice)
{
    auto sut = make_promise(&m_thread_pool, []() {});

    sut.resolve();
    ASSERT_ANY_THROW(sut.resolve());
}

TEST_F(Promise, ResolveMayBeCalledWithVariousParameters)
{
    event sync_event;
    auto sut = make_promise(&m_thread_pool, [&](int i, double d, char c) {
        EXPECT_EQ(i, 5);
        EXPECT_EQ(d, 10.0);
        EXPECT_EQ(c, 'v');
        sync_event.set();
    });

    sut.resolve(5, 10.0, 'v');
    
    sync_event.wait();
}

TEST_F(Promise, MoveParametersIntoResolveFunction)
{
    event sync_event;
    test_type param;
    auto sut = make_promise(&m_thread_pool, [&](test_type &&) {
        sync_event.set();
        return 0;
    });

    sut.resolve(std::move(param));

    sync_event.wait();
    EXPECT_TRUE(param.was_moved);
    EXPECT_EQ(test_type::copy_num, 0u);
    EXPECT_EQ(test_type::copy_assign_num, 0u);
    EXPECT_GT(test_type::move_num, 0u);
    EXPECT_EQ(test_type::move_assign_num, 0u);
}

TEST_F(Promise, CopyResolveFunctionParameterOnlyOnce)
{
    event sync_event;
    test_type param;
    auto sut = make_promise(&m_thread_pool, [&](const test_type &) {
        sync_event.set();
        return 0;
    });

    sut.resolve(param);

    sync_event.wait();
    EXPECT_FALSE(param.was_moved);
    EXPECT_EQ(test_type::copy_num, 1u);
    EXPECT_EQ(test_type::copy_assign_num, 0u);
    EXPECT_GT(test_type::move_num, 0u);
    EXPECT_EQ(test_type::move_assign_num, 0u);
}

TEST_F(Promise, MoveParametersIntoResolveFunctionIfFunctionReturnsVoid)
{
    event sync_event;
    test_type param;
    auto sut = make_promise(&m_thread_pool, [&](test_type &&) {
        sync_event.set();
    });

    sut.resolve(std::move(param));

    sync_event.wait();
    EXPECT_TRUE(param.was_moved);
    EXPECT_EQ(test_type::copy_num, 0u);
    EXPECT_EQ(test_type::copy_assign_num, 0u);
    EXPECT_GT(test_type::move_num, 0u);
    EXPECT_EQ(test_type::move_assign_num, 0u);
}

TEST_F(Promise, CopyResolveFunctionParameterOnlyOnceIfFunctionReturnsVoid)
{
    event sync_event;
    test_type param;
    auto sut = make_promise(&m_thread_pool, [&](const test_type &) {
        sync_event.set();
    });

    sut.resolve(param);

    sync_event.wait();
    EXPECT_FALSE(param.was_moved);
    EXPECT_EQ(test_type::copy_num, 1u);
    EXPECT_EQ(test_type::copy_assign_num, 0u);
    EXPECT_GT(test_type::move_num, 0u);
    EXPECT_EQ(test_type::move_assign_num, 0u);
}

TEST_F(Promise, FunctionMayReturnVoidType)
{
    event sync_event;
    auto sut = make_promise(&m_thread_pool, [&]() {
        sync_event.set();
    });

    sut.resolve();

    sync_event.wait();
}

TEST_F(Promise, FunctionMayReturnTypeWithAnyQualifiers)
{
    int i = 0;
    volatile int ii = 0;

    auto sut1 = make_promise(&m_thread_pool, [&]() -> int {
        return 1;
    }); sut1.resolve();
    auto sut2 = make_promise(&m_thread_pool, [&]() -> int & {
        return i;
    }); sut2.resolve();
    auto sut3 = make_promise(&m_thread_pool, [&]() -> int && {
        return std::move(i);
    }); sut3.resolve();
    auto sut4 = make_promise(&m_thread_pool, [&]() -> const int {
        return 1;
    }); sut4.resolve();
    auto sut5 = make_promise(&m_thread_pool, [&]() -> const int & {
        return i;
    }); sut5.resolve();
    auto sut6 = make_promise(&m_thread_pool, [&]() -> const int && {
        return std::move(i);
    }); sut6.resolve();
    auto sut7 = make_promise(&m_thread_pool, [&]() -> volatile int {
        return 1;
    }); sut7.resolve();
    auto sut8 = make_promise(&m_thread_pool, [&]() -> volatile int & {
        return ii;
    }); sut8.resolve();
    auto sut9 = make_promise(&m_thread_pool, [&]() -> volatile int && {
        return std::move(ii);
    }); sut9.resolve();
    auto sut10 = make_promise(&m_thread_pool, [&]() -> const volatile int {
        return 1;
    }); sut10.resolve();
    auto sut11 = make_promise(&m_thread_pool, [&]() -> const volatile int & {
        return ii;
    }); sut11.resolve();
    auto sut12 = make_promise(&m_thread_pool, [&]() -> const volatile int && {
        return std::move(ii);
    }); sut12.resolve();
}

TEST_F(Promise, FunctionMayParameterTypeWithAnyQualifierExceptNonConstLValueReference)
{
    int i = 0;
    volatile int ii = 0;

    auto sut1 = make_promise(&m_thread_pool, [&](int) { }); sut1.resolve(i);
    //auto sut2 = make_promise(&m_thread_pool, [&](int &) { }); sut2.resolve(i);
    auto sut3 = make_promise(&m_thread_pool, [&](int &&) { }); sut3.resolve(std::move(i));
    auto sut4 = make_promise(&m_thread_pool, [&](const int) { }); sut4.resolve(i);
    auto sut5 = make_promise(&m_thread_pool, [&](const int &) { }); sut5.resolve(i);
    auto sut6 = make_promise(&m_thread_pool, [&](const int &&) { }); sut6.resolve(std::move(i));
    auto sut7 = make_promise(&m_thread_pool, [&](volatile int) { }); sut7.resolve(ii);
    //auto sut8 = make_promise(&m_thread_pool, [&](volatile int &) { }); sut8.resolve(ii);
    auto sut9 = make_promise(&m_thread_pool, [&](volatile int &&) { }); sut9.resolve(std::move(ii));
    auto sut10 = make_promise(&m_thread_pool, [&](const volatile int) { }); sut10.resolve(ii);
    //auto sut11 = make_promise(&m_thread_pool, [&](const volatile int &) { }); sut11.resolve(ii); ???
    auto sut12 = make_promise(&m_thread_pool, [&](const volatile int &&) { }); sut12.resolve(std::move(ii));
}

TEST_F(Promise, FunctionMovesToPromise)
{
    event sync_event;
    auto f = std::make_unique<std::function<void()>>([t = test_type{}, &sync_event]() { sync_event.set(); });
    auto sut = make_promise(&m_thread_pool, std::move(*f));

    f.reset();
    sut.resolve();

    sync_event.wait();
    EXPECT_EQ(test_type::copy_num, 0u);
    EXPECT_EQ(test_type::copy_assign_num, 0u);
    EXPECT_GT(test_type::move_num, 0u);
    EXPECT_EQ(test_type::move_assign_num, 0u);
}

TEST_F(Promise, FunctionCopiesToPromiseOnlyOnce)
{
    event sync_event;
    auto f = std::make_unique<std::function<void()>>([t = test_type{}, &sync_event]() { sync_event.set(); });
    auto sut = make_promise(&m_thread_pool, *f);

    f.reset();
    sut.resolve();

    sync_event.wait();
    EXPECT_EQ(test_type::copy_num, 1u);
    EXPECT_EQ(test_type::copy_assign_num, 0u);
    EXPECT_GT(test_type::move_num, 0u);
    EXPECT_EQ(test_type::move_assign_num, 0u);
}

TEST_F(Promise, FunctionExecutesCorrectlyEvenAfterPromise)
{
    event sync_event1;
    event sync_event2;
    auto sut = std::make_unique<promise<thread_pool, void>>(make_promise(&m_thread_pool, [&]() {
        sync_event1.wait();
        sync_event2.set();
    }));

    sut->resolve();
    sut.reset();

    sync_event1.set();
    sync_event2.wait();
}
