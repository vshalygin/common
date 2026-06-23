#include <common-lib/thread/future.h>
#include <common-lib/thread/thread-pool.h>
#include <common-lib/synchronization/event/event.h>

#include <gtest/gtest.h>

#include <type_traits>
#include <atomic>
#include <utility>

using namespace vshalygin::cl;
using namespace testing;

//future is move only
static_assert(!std::is_copy_constructible_v<future<int>>);
static_assert(!std::is_copy_assignable_v<future<int>>);
static_assert(std::is_move_constructible_v<future<int>>);
static_assert(std::is_move_assignable_v<future<int>>);

//promise is move only
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
    auto data = future.get_data();
    data.apply([](int i) { ASSERT_EQ(i, 1); });
}

TEST_F(Future, ExecutesSuccessCallback)
{
    int i = 0;
    promise promise(&m_pool, []() { return 2; });
    auto future = promise.resolve();
    future.then([&i](int &&ii) { i = ii; return 0; })
          .get_data();

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
    future.get_data().apply([](int i) { ASSERT_EQ(i, 2); });
}

TEST_F(Future, FutureIsInvalidAfterMove)
{
    auto p = promise(&m_pool, []() { return 2; });
    auto future = p.resolve();
    auto other_future(std::move(future));

    ASSERT_FALSE(future.is_valid());
    ASSERT_TRUE(other_future.is_valid());
    other_future.get_data().apply([](int i) { ASSERT_EQ(i, 2); });
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
    future2.get_data().apply([](int i) { ASSERT_EQ(i, 1); });
}

TEST_F(Future, FutureIsValidAfterCorrespondingPromiseDestoyed)
{
    auto future = promise(&m_pool, []() { return 2; }).resolve();

    ASSERT_TRUE(future.is_valid());
    future.get_data().apply([](int i) { ASSERT_EQ(i, 2); });
}

TEST_F(Future, TestChaining)
{
    auto r = promise(&m_pool, []() { return 2; })
        .resolve()
        .then([](int i) { return i * 2; })
        .then([](int i) { return i + 1; })
        .get_data();

    r.apply([](int i) { ASSERT_EQ(i, 5); });
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
        .get_data();

    r.apply([](int i) { ASSERT_EQ(i, 9); });
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

    f.get_data().apply([](int i) { ASSERT_EQ(i, 2); });
}

TEST_F(Future, MayWorkOnThreadPoolWithoutMoveOnlyFunctorsSupport)
{
    thread_pool_wit_functor_copy_requirenment pool;
    auto f = promise<int, decltype(pool)>(&pool, []() { return 2; })
        .resolve()
        .then([](int i) { return i + 3; });

    f.get_data().apply([](int i) { ASSERT_EQ(i, 5); });
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
        f.get_data();
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
        f.get_data();
        FAIL();
    } catch(const std::runtime_error &e) {
        ASSERT_EQ(e.what(), std::string("message"));
    }

    try {
        f.then([&flag](int) { flag = true; return 0; }).get_data();
        FAIL();
    } catch(const std::runtime_error &e) {
        ASSERT_EQ(e.what(), std::string("message"));
    }

    ASSERT_FALSE(flag);
}

TEST_F(Future, MethodGetThrowsExceptionIfExecutingTasksThrows)
{
    auto f = promise(&m_pool, []() -> int { throw std::runtime_error("message"); })
        .resolve();

    try {
        f.get_data();
        FAIL();
    } catch (const std::runtime_error &e) {
        ASSERT_TRUE(e.what() == std::string("message"));
    }
}

TEST_F(Future, ExecuteSuccessHandlerIfThisHandlerSetAfterTaskExecution)
{
    auto f = promise(&m_pool, []() -> int { return 0; })
        .resolve();
    f.get_data();

    event sync_event;
    f.then([&](int) { sync_event.set(); return 0; });

    sync_event.wait();
}

TEST_F(Future, ExecuteErrorHandlerIfThisHandlerSetAfterTaskExecution)
{
    auto f = promise(&m_pool, []() -> int { throw std::runtime_error("message"); })
        .resolve();
    try {
        f.get_data();
    } catch(...) {
    }

    event sync_event;
    f.catched([&](std::exception_ptr e) {
        try {
            std::rethrow_exception(e);
        } catch(const std::exception &e) {
            ASSERT_TRUE(e.what() == std::string("message"));
            sync_event.set();
        }
    });

    sync_event.wait();
}

TEST_F(Future, CannotSetSuccessHandlerTwice)
{
    auto f = promise(&m_pool, []() -> int { return 0; })
        .resolve();

    f.then([](int) { return 0; });
    EXPECT_ANY_THROW(f.then([](int) { return 2; }));
}

TEST_F(Future, CannotSetFailHandlerTwice)
{
    auto f = promise(&m_pool, []() -> int { return 0; })
        .resolve();

    f.catched([](std::exception_ptr) {});
    EXPECT_ANY_THROW(f.catched([](std::exception_ptr) {}));
}

TEST_F(Future, CannotSetFailHandlerAfterSetSuccessHandler)
{
    auto f = promise(&m_pool, []() -> int { return 0; })
        .resolve();

    f.then([](int) { return 0; });
    EXPECT_ANY_THROW(f.catched([](std::exception_ptr) {}));
}

TEST_F(Future, SuccessHandlerExecutesIfCorrespondingFutureAndPromiseDestroyed)
{
    event sync_event1;
    event sync_event2;
    promise(&m_pool, [&]() -> int { sync_event1.wait(); return 0; })
        .resolve()
        .then([&](int) { sync_event2.set(); return 0; });

    sync_event1.set();
    sync_event2.wait();
}

TEST_F(Future, FailHandlerExecutesIfCorrespondingFutureAndPromiseDestroyed)
{
    event sync_event1;
    event sync_event2;
    promise(&m_pool, [&]() -> int { sync_event1.wait(); throw 1; })
        .resolve()
        .catched([&](std::exception_ptr) { sync_event2.set();});

    sync_event1.set();
    sync_event2.wait();
}

TEST_F(Future, PromiseFunctionNeverCopyIfMovedToPromiseConstructor)
{
    auto c = counter{};
    auto promise_task = [c = std::move(c)]() -> int {
        return 0;
    };
    auto f = promise(&m_pool, std::move(promise_task))
        .resolve()
        .then([&](int) { return 0; });

    f.get_data();

    EXPECT_EQ(counter::copy_num, 0u);
    EXPECT_EQ(counter::copy_assign_num, 0u);
    EXPECT_EQ(counter::move_num, 2u);
    EXPECT_EQ(counter::move_assign_num, 0u);
}

TEST_F(Future, PromiseFunctionMayBeCopiedToPromiseConstructor)
{
    event sync_event;

    std::function<int()> promise_task;
    auto c = counter{};
    promise_task = [c, &sync_event]()->int {
        sync_event.wait();
        c.do_something(); //check valid
        return 0;
    };
    auto f = promise(&m_pool, promise_task)
        .resolve()
        .then([&](int) { sync_event.wait(); return 0; });
    promise_task = {};
    sync_event.set();

    f.get_data();

    EXPECT_EQ(counter::copy_num, 2u);
    EXPECT_EQ(counter::copy_assign_num, 0u);
    EXPECT_EQ(counter::move_num, 1u);
    EXPECT_EQ(counter::move_assign_num, 0u);
}

TEST_F(Future, SuccessHandlerNeverCopyIfMovedToThenMethod)
{
    auto c = counter{};
    auto task = [c = std::move(c)](int) -> int {
        return 0;
    };
    auto f = promise(&m_pool, []() { return 1; })
        .resolve()
        .then(std::move(task));

    f.get_data();

    EXPECT_EQ(counter::copy_num, 0u);
    EXPECT_EQ(counter::copy_assign_num, 0u);
    EXPECT_EQ(counter::move_num, 3u);
    EXPECT_EQ(counter::move_assign_num, 0u);
}

TEST_F(Future, SuccessMayBeCopiedToThan)
{
    event sync_event;

    std::function<int(int)> task;
    auto c = counter{};
    task = [c, &sync_event](int)->int {
        sync_event.wait();
        c.do_something(); //check valid
        return 0;
    };
    auto f = promise(&m_pool, []() { return 0; })
        .resolve()
        .then(task);
    task = {};
    sync_event.set();

    f.get_data();

    EXPECT_EQ(counter::copy_num, 2u);
    EXPECT_EQ(counter::copy_assign_num, 0u);
    EXPECT_EQ(counter::move_num, 1u);
    EXPECT_EQ(counter::move_assign_num, 0u);
}
