#include <common-lib/thread/future.h>
#include <common-lib/thread/thread-pool.h>
#include <common-lib/synchronization/event/event.h>

#include <gtest/gtest.h>

#include <type_traits>
#include <atomic>
#include <utility>

using namespace vshalygin::cl;
using namespace testing;

static_assert(!std::is_copy_constructible_v<future<int>>);
static_assert(!std::is_copy_assignable_v<future<int>>);
static_assert(std::is_move_constructible_v<future<int>>);
static_assert(std::is_move_assignable_v<future<int>>);

static_assert(!std::is_copy_constructible_v<promise<int>>);
static_assert(!std::is_copy_assignable_v<promise<int>>);
static_assert(std::is_move_constructible_v<promise<int>>);
static_assert(std::is_move_assignable_v<promise<int>>);

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

    class thread_pool_wit_functor_copy_requirenment
    {
    public:
        void post(std::function<void()> &&task)
        {
            m_pool.post(std::move(task));
        }

    private:
        thread_pool m_pool{ 2 };
    };
}

class Future
    : public Test
{
protected:
    void SetUp() override
    {
        counter::clear();
    }

protected:
    thread_pool m_pool{ 2 };
};

TEST_F(Future, Init)
{
    promise promise(&m_pool, []() -> int { return 1; });
    auto future = promise.resolve();
    ASSERT_EQ(future.get(), 1);
}

TEST_F(Future, ExecutesSuccessCallback)
{
    int i = 0;
    promise promise(&m_pool, []() { return 2; });
    auto future = promise.resolve();
    future.then([&i](int &&ii) { i = ii; return 0; })
          .get();

    ASSERT_EQ(i, 2);
}

TEST_F(Future, PromiseIsValidAfterCreation)
{
    promise sut(&m_pool, []() { return 1; });

    ASSERT_TRUE(sut.is_valid());
}

TEST_F(Future, PromiseIsNotValidAfterMove)
{
    promise sut(&m_pool, []() { return 1; });
    promise other(std::move(sut));

    EXPECT_TRUE(other.is_valid());
    EXPECT_FALSE(sut.is_valid());
}

TEST_F(Future, PromiseIsNotValidAfterMoveAssignment)
{
    promise sut(&m_pool, []() { return 1; });
    promise other(&m_pool, []() { return 1; });

    other = std::move(sut);

    EXPECT_TRUE(other.is_valid());
    ASSERT_FALSE(sut.is_valid());
}

TEST_F(Future, DefaultCreatedPromiseIsNotValid)
{
    promise<int> sut;

    ASSERT_FALSE(sut.is_valid());
}

TEST_F(Future, DefaultCreatedPromiseIsValidAfterAssigningValidPromise)
{
    promise<int> sut;
    sut = promise<int>(&m_pool, []() { return 0; });

    ASSERT_TRUE(sut.is_valid());
}

TEST_F(Future, DefaultCreatedFutureIsNotValid)
{
    future<int> sut;

    ASSERT_FALSE(sut.is_valid());
}

TEST_F(Future, FutureCreatedFromPromiseIsValid)
{
    auto p = promise(&m_pool, []() { return 2; });
    auto future = p.resolve();

    ASSERT_TRUE(future.is_valid());
    ASSERT_EQ(future.get(), 2);
}

TEST_F(Future, FutureIsInvalidAfterMove)
{
    auto p = promise(&m_pool, []() { return 2; });
    auto future = p.resolve();
    auto other_future(std::move(future));

    ASSERT_FALSE(future.is_valid());
    ASSERT_TRUE(other_future.is_valid());
    ASSERT_EQ(other_future.get(), 2);
}

TEST_F(Future, FutureIsInvalidAfterMoveAssign)
{
    auto p1 = promise(&m_pool, []() { return 1; });
    auto p2 = promise(&m_pool, []() { return 2; });
    auto future1 = p1.resolve();
    auto future2 = p2.resolve();
    future2 = std::move(future1);

    ASSERT_FALSE(future1.is_valid());
    ASSERT_TRUE(future2.is_valid());
    ASSERT_EQ(future2.get(), 1);
}

TEST_F(Future, FutureIsValidAfterCorrespondingPromiseDestoyed)
{
    auto future = promise(&m_pool, []() { return 2; }).resolve();

    ASSERT_TRUE(future.is_valid());
    ASSERT_EQ(future.get(), 2);
}

TEST_F(Future, TestChaining)
{
    auto r = promise(&m_pool, []() { return 2; })
        .resolve()
        .then([](int i) { return i * 2; })
        .then([](int i) { return i + 1; })
        .get();

    ASSERT_EQ(r, 5);
}

TEST_F(Future, CatchesExeption)
{
    event sync_event;
    promise(&m_pool, []()->int{ throw std::runtime_error("message"); })
        .resolve()
        .catched([&sync_event](std::exception_ptr e) {
            try {
                std::rethrow_exception(e);
            } catch(const std::runtime_error &e) {
                EXPECT_EQ(e.what(), std::string("message"));
                sync_event.set();
            }
        });

    sync_event.wait();
}

TEST_F(Future, ExceptionCatchedInChainedHanler)
{
    event sync_event;
    promise(&m_pool, []() { return 2; })
        .resolve()
        .then([](int)->int { throw std::runtime_error("message"); })
        .then([](int i) { return i + 1; })
        .catched([&sync_event](std::exception_ptr e) {
            try{
                std::rethrow_exception(e);
            } catch(const std::runtime_error &e) {
                EXPECT_EQ(e.what(), std::string("message"));
                sync_event.set();
            }
        });

    sync_event.wait();
}

TEST_F(Future, ExceptionCatchedInClosestChainedHanler)
{
    event sync_event;
    promise(&m_pool, []() { return 2; })
        .resolve()
        .then([](int)->int { throw std::runtime_error("message"); })
        .catched([&sync_event](std::exception_ptr e) {
                     try {
                         std::rethrow_exception(e);
                     } catch(const std::runtime_error &e) {
                         EXPECT_EQ(e.what(), std::string("message"));
                         sync_event.set();
                     }
                 })
        .then([](int i) { return i + 1; })
        .catched([](std::exception_ptr) { FAIL(); });

    sync_event.wait();
}

TEST_F(Future, IgnorePassedExceptionHandler)
{
    event sync_event;
    promise(&m_pool, []() { return 2; })
        .resolve()
        .then([](int) { return 0; })
        .catched([](std::exception_ptr) { FAIL(); })
        .then([](int)->int{ throw std::runtime_error("message"); })
        .catched([&sync_event](std::exception_ptr e) {
            try{
                std::rethrow_exception(e);
            } catch(const std::runtime_error &e) {
                EXPECT_EQ(e.what(), std::string("message"));
                sync_event.set();
            }
        });

    sync_event.wait();
}

TEST_F(Future, MayGetValueAfterCatchHandlerSet)
{
    auto r = promise(&m_pool, []() { return 2; })
        .resolve()
        .then([](int i) { return i + 3; })
        .catched([](std::exception_ptr) { FAIL(); })
        .then([](int i)->int { return i + 4; })
        .catched([](std::exception_ptr) { FAIL(); })
        .get();

    ASSERT_EQ(r, 9);
}

TEST_F(Future, CatchedMethodAppliedToLValue)
{
    auto f = promise(&m_pool, []() { return 2; }).resolve();
    auto res = std::is_same_v<
        decltype(f.catched(std::declval<std::function<void(std::exception_ptr)>>())),
        future<int> &>;

    ASSERT_TRUE(res);
}

TEST_F(Future, CatchedMethodAppliedToRValue)
{
    auto f = promise(&m_pool, []() { return 2; })
        .resolve()
        .catched([](std::exception_ptr) { FAIL(); });

    ASSERT_EQ(f.get(), 2);
}

TEST_F(Future, MayWorkOnThreadPoolWithoutMoveOnlyFunctorsSupport)
{
    thread_pool_wit_functor_copy_requirenment pool;
    auto f = promise<int, decltype(pool)>(&pool, []() { return 2; })
        .resolve()
        .then([](int i) { return i + 3; });

    ASSERT_EQ(f.get(), 5);
}

TEST_F(Future, DoNotExecuteChandedHandlersIfPreviousWasInterruptedByException)
{
    bool flag = false;
    event sync_event;
    promise(&m_pool, []() { return 2; })
        .resolve()
        .then([](int) { return 0; })
        .then([](int)->int { throw std::runtime_error("message"); })
        .then([&](int)->int { flag = true; return 0; })
        .catched([&sync_event](std::exception_ptr e) {
                     try {
                         std::rethrow_exception(e);
                     }
                     catch(const std::runtime_error &e) {
                         EXPECT_EQ(e.what(), std::string("message"));
                         sync_event.set();
                     }
                 });

    sync_event.wait();
    ASSERT_FALSE(flag);
}

TEST_F(Future, NextChainedFutureAfterFailedFutureExcutesFailHandler)
{
    bool flag = false;
    event sync_event;
    auto f = promise(&m_pool, []() { return 2; })
        .resolve()
        .then([](int) { return 0; })
        .then([](int)->int { throw std::runtime_error("message"); });
    try {
        f.get();
        FAIL();
    } catch (const std::runtime_error& e) {
        ASSERT_EQ(e.what(), std::string("message"));
    }
    f.then([&](int)->int { flag = true; return 0; })
     .catched([&sync_event](std::exception_ptr e) {
                     try {
                         std::rethrow_exception(e);
                     } catch(const std::runtime_error &e) {
                         EXPECT_EQ(e.what(), std::string("message"));
                         sync_event.set();
                     }
                 });

    sync_event.wait();
    ASSERT_FALSE(flag);
}

TEST_F(Future, NextChainedFutureAfterFailedFutureThrowsOnGet)
{
    bool flag = false;
    auto f = promise(&m_pool, []() { return 2; })
        .resolve()
        .then([](int) { return 0; })
        .then([](int)->int { throw std::runtime_error("message"); });
    try {
        f.get();
        FAIL();
    } catch(const std::runtime_error &e) {
        ASSERT_EQ(e.what(), std::string("message"));
    }

    try {
        f.then([&flag](int) { flag = true; return 0; }).get();
        FAIL();
    } catch(const std::runtime_error &e) {
        ASSERT_EQ(e.what(), std::string("message"));
    }

    ASSERT_FALSE(flag);
}
