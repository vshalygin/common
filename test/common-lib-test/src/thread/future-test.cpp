#include <common-lib/thread/future/future.h>
#include <common-lib/thread/thread-pool/thread-pool.h>
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
            return *this;
        }

        counter(counter &&)
        {
            ++move_num;
        }

        counter &operator=(counter &&)
        {
            ++move_assign_num;
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
    promise.resolve();
    auto future = promise.get_future();
    auto data = future.get();
    data.apply([](int i) { ASSERT_EQ(i, 1); });
}

TEST_F(Future, ExecutesSuccessCallback)
{
    int i = 0;
    promise promise(&m_pool, []() { return 2; });
    promise.resolve();
    auto future = promise.get_future();
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
    sut = promise(&m_pool, []() { return 0; });

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
    p.resolve();
    auto future = p.get_future();

    ASSERT_TRUE(future.is_valid());
    future.get().apply([](int i) { ASSERT_EQ(i, 2); });
}

TEST_F(Future, FutureIsInvalidAfterMove)
{
    auto p = promise(&m_pool, []() { return 2; });
    p.resolve();
    auto future = p.get_future();
    auto other_future(std::move(future));

    ASSERT_FALSE(future.is_valid());
    ASSERT_TRUE(other_future.is_valid());
    other_future.get().apply([](int i) { ASSERT_EQ(i, 2); });
}

TEST_F(Future, FutureIsInvalidAfterMoveAssign)
{
    auto p1 = promise(&m_pool, []() { return 1; });
    auto p2 = promise(&m_pool, []() { return 2; });
    p1.resolve();
    p2.resolve();
    auto future1 = p1.get_future();
    auto future2 = p2.get_future();
    future2 = std::move(future1);

    ASSERT_FALSE(future1.is_valid());
    ASSERT_TRUE(future2.is_valid());
    future2.get().apply([](int i) { ASSERT_EQ(i, 1); });
}

TEST_F(Future, FutureIsValidAfterCorrespondingPromiseDestoyed)
{
    auto p = std::make_unique<promise<int>>(&m_pool, []() { return 2; });
    p->resolve();
    auto future = p->get_future();
    p.reset();

    ASSERT_TRUE(future.is_valid());
    future.get().apply([](int i) { ASSERT_EQ(i, 2); });
}

TEST_F(Future, TestChaining)
{
    promise promice(&m_pool, []() { return 2; });
    promice.resolve();

    auto r = promice.get_future()
        .then([](int i) { return i * 2; })
        .then([](int i) { return i + 1; })
        .get();

    r.apply([](int i) { ASSERT_EQ(i, 5); });
}

TEST_F(Future, CatchesException)
{
    event sync_event;
    promise p(&m_pool, []()->int { throw std::runtime_error("message"); });
    p.resolve();
    p.get_future()
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
    promise p(&m_pool, []() { return 2; });
    p.get_future()
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
    p.resolve();

    sync_event.wait();
}

TEST_F(Future, ExceptionCatchedInClosestChainedHanler)
{
    event sync_event;
    promise p(&m_pool, []() { return 2; });
    p.resolve();
    p.get_future()
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
    promise p(&m_pool, []() { return 2; });
    p.resolve();
    p.get_future()
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
    promise p(&m_pool, []() { return 2; });
    p.resolve();
    auto r = p.get_future()
        .then([](int i) { return i + 3; })
        .catched([](std::exception_ptr) { FAIL(); })
        .then([](int i)->int { return i + 4; })
        .catched([](std::exception_ptr) { FAIL(); })
        .get();

    r.apply([](int i) { ASSERT_EQ(i, 9); });
}

TEST_F(Future, CatchedMethodAppliedToLValue)
{
    promise p(&m_pool, []() { return 2; });
    p.resolve();
    auto f = p.get_future();
    auto res = std::is_same_v<
        decltype(f.catched(std::declval<std::function<void(std::exception_ptr)>>())),
        future<int> &>;

    ASSERT_TRUE(res);
}

TEST_F(Future, CatchedMethodAppliedToRValue)
{
    promise p(&m_pool, []() { return 2; });
    p.resolve();
    auto f = p.get_future()
        .then([](int) { return 3; })
        .catched([](std::exception_ptr) { FAIL(); });

    f.get().apply([](int i) { ASSERT_EQ(i, 3); });
}

TEST_F(Future, MayWorkOnThreadPoolWithoutMoveOnlyFunctorsSupport)
{
    thread_pool_wit_functor_copy_requirenment pool;
    promise p(&pool, []() { return 2; });
    p.resolve();
    auto f = p.get_future()
        .then([](int i) { return i + 3; });

    f.get().apply([](int i) { ASSERT_EQ(i, 5); });
}

TEST_F(Future, DoNotExecuteChandedHandlersIfPreviousWasInterruptedByException)
{
    bool flag = false;
    event sync_event;
    promise p(&m_pool, []() { return 2; });
    p.resolve();
    p.get_future()
        .then([](int) { return 0; })
        .then([](int)->int { throw std::runtime_error("message"); })
        .then([&](int)->int { flag = true; return 0; })
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

TEST_F(Future, NextChainedFutureAfterFailedFutureExcutesFailHandler)
{
    bool flag = false;
    event sync_event;
    promise p(&m_pool, []() { return 2; });
    p.resolve();
    auto f = p.get_future()
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
    promise p(&m_pool, []() { return 2; });
    p.resolve();
    auto f = p.get_future()
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

TEST_F(Future, MethodGetThrowsExceptionIfExecutingTasksThrows)
{
    promise p(&m_pool, []() -> int { throw std::runtime_error("message"); });
    p.resolve();
    auto f = p.get_future();

    try {
        f.get();
        FAIL();
    } catch (const std::runtime_error &e) {
        ASSERT_TRUE(e.what() == std::string("message"));
    }
}

TEST_F(Future, ExecuteSuccessHandlerIfThisHandlerSetAfterTaskExecution)
{
    promise p(&m_pool, []() -> int { return 0; });
    auto f = p.get_future();
    p.resolve();
    f.get();

    event sync_event;
    f.then([&](int) { sync_event.set(); return 0; });

    sync_event.wait();
}

TEST_F(Future, ExecuteErrorHandlerIfThisHandlerSetAfterTaskExecution)
{
    promise p(&m_pool, []() -> int { throw std::runtime_error("message"); });
    p.resolve();
    auto f = p.get_future();
    try {
        f.get();
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
    promise p(&m_pool, []() -> int { return 0; });
    p.resolve();
    auto f = p.get_future();

    f.then([](int) { return 0; });
    EXPECT_ANY_THROW(f.then([](int) { return 2; }));
}

TEST_F(Future, CannotSetFailHandlerTwice)
{
    promise p(&m_pool, []() -> int { return 0; });
    p.resolve();
    auto f = p.get_future();

    f.catched([](std::exception_ptr) {});
    EXPECT_ANY_THROW(f.catched([](std::exception_ptr) {}));
}

TEST_F(Future, CannotSetFailHandlerAfterSetSuccessHandler)
{
    promise p(&m_pool, []() -> int { return 0; });
    p.resolve();
    auto f = p.get_future();

    f.then([](int) { return 0; });
    EXPECT_ANY_THROW(f.catched([](std::exception_ptr) {}));
}

TEST_F(Future, SuccessHandlerExecutesIfCorrespondingFutureAndPromiseDestroyed)
{
    event sync_event1;
    event sync_event2;
    promise p(&m_pool, [&]() -> int { sync_event1.wait(); return 0; });
    p.resolve();
    p.get_future()
        .then([&](int) { sync_event2.set(); return 0; });

    sync_event1.set();
    sync_event2.wait();
}

TEST_F(Future, FailHandlerExecutesIfCorrespondingFutureAndPromiseDestroyed)
{
    event sync_event1;
    event sync_event2;
    promise p(&m_pool, [&]() -> int { sync_event1.wait(); throw 1; });
    p.resolve();
    p.get_future()
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
    promise p(&m_pool, std::move(promise_task));
    p.resolve();
    auto f = p.get_future()
        .then([&](int) { return 0; });

    f.get();

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
    promise p(&m_pool, promise_task);
    p.resolve();

    auto f = p.get_future()
        .then([&](int) { sync_event.wait(); return 0; });
    promise_task = {};
    sync_event.set();

    f.get();

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
    promise p(&m_pool, []() { return 1; });
    p.resolve();
    auto f = p.get_future()
        .then(std::move(task));

    f.get();

    EXPECT_EQ(counter::copy_num, 0u);
    EXPECT_EQ(counter::copy_assign_num, 0u);
    EXPECT_EQ(counter::move_num, 3u);
    EXPECT_EQ(counter::move_assign_num, 0u);
}

TEST_F(Future, SuccessHanlerMayBeCopiedToThenMethod)
{
    event sync_event;

    std::function<int(int)> task;
    auto c = counter{};
    task = [c, &sync_event](int)->int {
        sync_event.wait();
        c.do_something(); //check valid
        return 0;
    };
    promise p(&m_pool, []() { return 0; });
    p.resolve();
    auto f = p.get_future()
        .then(task);
    task = {};
    sync_event.set();

    f.get();

    EXPECT_EQ(counter::copy_num, 2u);
    EXPECT_EQ(counter::copy_assign_num, 0u);
    EXPECT_EQ(counter::move_num, 1u);
    EXPECT_EQ(counter::move_assign_num, 0u);
}

TEST_F(Future, FutureDataMayApplyHandlerWithOneParamerterWithVariousQualificators)
{
    static volatile int vi = 1;
    promise p1(&m_pool, []()->int { return 1; });
    promise p2(&m_pool, []()->volatile int &{ return vi; });
    p1.resolve();
    p2.resolve();
    auto f1 = p1.get_future();
    auto f2 = p2.get_future();

    auto data1 = f1.get();
    auto data2 = f2.get();
    data1.apply([](int i) { ASSERT_EQ(i, 1); });
    data1.apply([](int &i) { ASSERT_EQ(i, 1); });
    data1.apply([](int &&i) { ASSERT_EQ(i, 1); });
    data1.apply([](const int i) { ASSERT_EQ(i, 1); });
    data1.apply([](const int &i) { ASSERT_EQ(i, 1); });
    data1.apply([](const int &&i) { ASSERT_EQ(i, 1); });
    data2.apply([](volatile int i) { ASSERT_EQ(i, 1); });
    data2.apply([](volatile int &i) { ASSERT_EQ(i, 1); });
    data2.apply([](volatile int &&i) { ASSERT_EQ(i, 1); });
    data2.apply([](volatile const int i) { ASSERT_EQ(i, 1); });
    data2.apply([](volatile const int &i) { ASSERT_EQ(i, 1); });
    data2.apply([](volatile const int &&i) { ASSERT_EQ(i, 1); });
    
    data1.apply([](int &i) { i = 2; });
    data1.apply([](const int &i) { ASSERT_EQ(i, 2); });
    data2.apply([](volatile int &i) { i = 2; });
    data2.apply([](volatile const int &i) { ASSERT_EQ(i, 2); });
}

TEST_F(Future, ConstFutureDataMayApplyHandlerWithNonModifyOneParameter)
{
    static volatile int vi = 4;

    promise p1(&m_pool, []()->int { return 1; });
    p1.resolve();
    auto f1 = p1.get_future();

    const auto data1 = f1.get();
    data1.apply([](int i) { ASSERT_EQ(i, 1); });
    data1.apply([](int &i) { ASSERT_EQ(i, 1); });
    data1.apply([](int &&i) { ASSERT_EQ(i, 1); });
    data1.apply([](const int i) { ASSERT_EQ(i, 1); });
    data1.apply([](const int &i) { ASSERT_EQ(i, 1); });
    data1.apply([](const int &&i) { ASSERT_EQ(i, 1); });
    data1.apply([](volatile int i) { ASSERT_EQ(i, 1); });
    data1.apply([](volatile int &i) { ASSERT_EQ(i, 1); });
    data1.apply([](volatile int &&i) { ASSERT_EQ(i, 1); });
    data1.apply([](volatile const int i) { ASSERT_EQ(i, 1); });
    data1.apply([](volatile const int &i) { ASSERT_EQ(i, 1); });
    data1.apply([](volatile const int &&i) { ASSERT_EQ(i, 1); });
}

TEST_F(Future, FutureDataMakeCopyOfValueIfFunctorHasNonReferenceParameter)
{
    promise p(&m_pool, []()->std::string { return "data"; });
    p.resolve();
    auto f = p.get_future();

    auto data = f.get();
    data.apply([](std::string i) { EXPECT_EQ(i, "data"); i[0] = 's'; });
    data.apply([](const std::string i) { EXPECT_EQ(i, "data"); });

    data.apply([](const std::string &i) { EXPECT_EQ(i, "data"); });
}

TEST_F(Future, FutureDataMakeAcceptRValueRefParameteraziedFunctorAndMoveValue)
{
    promise p(&m_pool, []()->std::string { return "data"; });
    p.resolve();
    auto f1 = p.get_future();
    auto data1 = f1.get();
    std::string acceptor1;
    data1.apply([&](std::string &&i) {
        acceptor1 = std::move(i);
    });

    EXPECT_EQ(acceptor1, "data");
    data1.apply([&](const std::string &i) { EXPECT_EQ(i, ""); });
}

TEST_F(Future, FutureDataMakeAcceptLValueRefParameteraziedFunctorAndChangeValue)
{
    promise p(&m_pool, []()->std::string { return "data"; });
    p.resolve();
    auto f1 = p.get_future();
    auto data1 = f1.get();
    data1.apply([&](std::string &i) { i[0] = 's'; });

    data1.apply([&](const std::string &i) { EXPECT_EQ(i, "sata"); });
}

TEST_F(Future, ConstFutureDataMakeCopyOfValueIfFunctorHasNonReferenceParameter)
{
    promise p(&m_pool, []()->std::string { return "data"; });
    p.resolve();
    auto f = p.get_future();

    const auto data = f.get();
    data.apply([](std::string i) { EXPECT_EQ(i, "data"); });
    data.apply([](const std::string i) { EXPECT_EQ(i, "data"); });

    data.apply([](const std::string &i) { EXPECT_EQ(i, "data"); });
}

TEST_F(Future, PassMoveOnlyTypeThroughChainHanlers)
{
    promise p(&m_pool, []()->std::unique_ptr<int> { return std::make_unique<int>(3); });
    p.resolve();
    auto d = p.get_future()
                 .then([](std::unique_ptr<int> &&i) { return std::move(i); })
                 .then([](std::unique_ptr<int> &&i) { return std::move(i); })
                 .then([](std::unique_ptr<int> &&i) { return std::move(i); })
                 .get();
    
    std::unique_ptr<int> acceptor;
    d.apply([&](std::unique_ptr<int> &&i) { acceptor = std::move(i); });
    
    ASSERT_TRUE(acceptor);
    ASSERT_EQ(*acceptor, 3);
}

TEST_F(Future, MayBeParameterizedByTypeWithAnyQualifiers)
{
    static int i2 = 2;
    static int i3 = 3;
    static int i5 = 5;
    static int i6 = 6;
    static volatile int i8 = 8;
    static volatile int i9 = 9;
    static volatile int i11 = 11;
    static volatile int i12 = 12;

    auto p1 = promise(&m_pool, []()->int { return 1; });
    p1.resolve();
    auto p2 = promise(&m_pool, []()->int &{ return i2; });
    p2.resolve();
    auto p3 = promise(&m_pool, []()->int &&{ return std::move(i3); });
    p3.resolve();
    auto p4 = promise(&m_pool, []()->const int { return 4; });
    p4.resolve();
    auto p5 = promise(&m_pool, []()->const int &{ return i5; });
    p5.resolve();
    auto p6 = promise(&m_pool, []()->const int &&{ return std::move(i6); });
    p6.resolve();
    auto p7 = promise(&m_pool, []()->volatile int { return 7; });
    p7.resolve();
    auto p8 = promise(&m_pool, []()->volatile int &{ return i8; });
    p8.resolve();
    auto p9 = promise(&m_pool, []()->volatile int &&{ return std::move(i9); });
    p9.resolve();
    auto p10 = promise(&m_pool, []()->const volatile int { return 10; });
    p10.resolve();
    auto p11 = promise(&m_pool, []()->const volatile int &{ return i11; });
    p11.resolve();
    auto p12 = promise(&m_pool, []()->const volatile int &&{ return std::move(i12); });
    p12.resolve();
    
    auto d1 = p1.get_future();
    auto d2 = p2.get_future();
    auto d3 = p3.get_future();
    auto d4 = p4.get_future();
    auto d5 = p5.get_future();
    auto d6 = p6.get_future();
    auto d7 = p7.get_future();
    auto d8 = p8.get_future();
    auto d9 = p9.get_future();
    auto d10 = p10.get_future();
    auto d11 = p11.get_future();
    auto d12 = p12.get_future();
    
    d1.get().apply([](int &i) { i++; });
    d2.get().apply([](int &i) { i++; });
    d3.get().apply([](int &&i) { i++; });
    d4.get().apply([](const int &) {});
    d5.get().apply([](const int &) {});
    d6.get().apply([](const int &&) {});
    d7.get().apply([](volatile int &i) { i++; });
    d8.get().apply([](volatile int &i) { i++; });
    d9.get().apply([](volatile int &&i) { i++; });
    d10.get().apply([](const volatile int &) {});
    d11.get().apply([](const volatile int &) {});
    d12.get().apply([](const volatile int &&) {});
    
    d1.get().apply([](int &i) { EXPECT_EQ(2, i); });
    d2.get().apply([](int &i) { EXPECT_EQ(3, i); });
    d3.get().apply([](int &&i) { EXPECT_EQ(4, i); });
    d4.get().apply([](const int &i) { EXPECT_EQ(4, i); });
    d5.get().apply([](const int &i) { EXPECT_EQ(5, i); });
    d6.get().apply([](const int &&i) { EXPECT_EQ(6, i); });
    d7.get().apply([](volatile int &i) { EXPECT_EQ(8, i); });
    d8.get().apply([](volatile int &i) { EXPECT_EQ(9, i); });
    d9.get().apply([](volatile int &&i) { EXPECT_EQ(10, i); });
    d10.get().apply([](const volatile int  &i) { EXPECT_EQ(10, i); });
    d11.get().apply([](const volatile int &i) { EXPECT_EQ(11, i); });
    d12.get().apply([](const volatile int &&i) { EXPECT_EQ(12, i); });
    EXPECT_EQ(i2, 3);
    EXPECT_EQ(i3, 4);
    EXPECT_EQ(i5, 5);
    EXPECT_EQ(i6, 6);
    EXPECT_EQ(i8, 9);
    EXPECT_EQ(i9, 10);
    EXPECT_EQ(i11, 11);
    EXPECT_EQ(i12, 12);
}

TEST_F(Future, PromiseCannotExecuteGetFutureTwice)
{
    promise p(&m_pool, []() { return 1; });
    p.get_future();
    ASSERT_ANY_THROW(p.get_future());
}

TEST_F(Future, PromiseCannotExecuteResolveTwice)
{
    promise p(&m_pool, []() { return 1; });
    p.resolve();
    ASSERT_ANY_THROW(p.resolve());
}

TEST_F(Future, DoesNotCopyMovableObjectInInnerStorage)
{
    promise p(&m_pool, []() { return counter{}; });
    p.resolve();
    auto f = p.get_future()
        .then([](counter &&c) { return std::move(c); })
        .then([](counter &&c) { return c; });

    counter acceptor;
    f.get().apply([&acceptor](counter &&c) { acceptor = std::move(c); });

    EXPECT_EQ(counter::copy_num, 0u);
    EXPECT_EQ(counter::copy_assign_num, 0u);
    EXPECT_TRUE(counter::move_num > 0u);
    EXPECT_TRUE(counter::move_assign_num >= 0);
}

TEST_F(Future, CallbacksMayReturnVoidType)
{
    promise p(&m_pool, []() {});
    p.resolve();
    auto f = p.get_future()
                .then([](){});

    f.get();
}

//TEST_F(Future, DoChainingWithVoidReturnCallback)
//{
//    int i = 0;
//    promise p(&m_pool, []() {});
//    p.resolve();
//    auto f = p.get_future()
//        .then([]() {})
//        .then([]() {})
//        .then([]() { return 1; });
//        //.then([](int) {})
//        //.then([]() {return 22; })
//        //.then([&i](int ii) { i = ii; });
//
//    f.get();
//    ASSERT_EQ(i, 22);
//}
//