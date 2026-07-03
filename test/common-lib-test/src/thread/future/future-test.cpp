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
static_assert(!std::is_copy_constructible_v<future<thread_pool, int>>);
static_assert(!std::is_copy_assignable_v<future<thread_pool, int>>);
static_assert(std::is_move_constructible_v<future<thread_pool, int>>);
static_assert(std::is_move_assignable_v<future<thread_pool, int>>);

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

    class thread_pool_with_functor_copy_requirement
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
    auto promise = make_promise(&m_pool, []() -> int { return 1; });
    promise.resolve();
    auto future = promise.get_future();
    auto data = future.get();
    data.apply([](int i) { ASSERT_EQ(i, 1); });
}

TEST_F(Future, ExecutesSuccessCallback)
{
    int i = 0;
    auto promise = make_promise(&m_pool, []() { return 2; });
    promise.resolve();
    auto future = promise.get_future();
    future.then([&i](int &&ii) { i = ii; return 0; })
          .get();

    ASSERT_EQ(i, 2);
}

TEST_F(Future, PromiseIsValidAfterCreation)
{
    auto sut = make_promise(&m_pool, []() { return 1; });

    ASSERT_TRUE(sut.is_valid());
}

TEST_F(Future, PromiseIsNotValidAfterMove)
{
    auto sut = make_promise(&m_pool, []() { return 1; });
    auto other(std::move(sut));

    EXPECT_TRUE(other.is_valid());
    EXPECT_FALSE(sut.is_valid());
}

TEST_F(Future, PromiseIsNotValidAfterMoveAssignment)
{
    auto sut = make_promise(&m_pool, []() { return 1; });
    auto other = make_promise(&m_pool, []() { return 1; });

    other = std::move(sut);

    EXPECT_TRUE(other.is_valid());
    ASSERT_FALSE(sut.is_valid());
}

TEST_F(Future, DefaultCreatedPromiseIsNotValid)
{
    promise<thread_pool, int> sut;

    ASSERT_FALSE(sut.is_valid());
}

TEST_F(Future, DefaultCreatedPromiseIsValidAfterAssigningValidPromise)
{
    promise<thread_pool, int> sut;
    sut = make_promise(&m_pool, []() { return 0; });

    ASSERT_TRUE(sut.is_valid());
}

TEST_F(Future, DefaultCreatedFutureIsNotValid)
{
    future<int, thread_pool> sut;

    ASSERT_FALSE(sut.is_valid());
}

TEST_F(Future, FutureCreatedFromPromiseIsValid)
{
    auto p = make_promise(&m_pool, []() { return 2; });
    p.resolve();
    auto future = p.get_future();

    ASSERT_TRUE(future.is_valid());
    future.get().apply([](int i) { ASSERT_EQ(i, 2); });
}

TEST_F(Future, FutureIsInvalidAfterMove)
{
    auto p = make_promise(&m_pool, []() { return 2; });
    p.resolve();
    auto future = p.get_future();
    auto other_future(std::move(future));

    ASSERT_FALSE(future.is_valid());
    ASSERT_TRUE(other_future.is_valid());
    other_future.get().apply([](int i) { ASSERT_EQ(i, 2); });
}

TEST_F(Future, FutureIsInvalidAfterMoveAssign)
{
    auto p1 = make_promise(&m_pool, []() { return 1; });
    auto p2 = make_promise(&m_pool, []() { return 2; });
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
    auto p = std::make_unique<promise<thread_pool, int>>(make_promise(&m_pool, []() { return 2; }));
    p->resolve();
    auto future = p->get_future();
    p.reset();

    ASSERT_TRUE(future.is_valid());
    future.get().apply([](int i) { ASSERT_EQ(i, 2); });
}

TEST_F(Future, TestChaining)
{
    auto promice = make_promise(&m_pool, []() { return 2; });
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
    auto p = make_promise(&m_pool, []()->int { throw std::runtime_error("message"); });
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
    auto p = make_promise(&m_pool, []() { return 2; });
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
    auto p = make_promise(&m_pool, []() { return 2; });
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
    auto p = make_promise(&m_pool, []() { return 2; });
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
    auto p = make_promise(&m_pool, []() { return 2; });
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
    auto p = make_promise(&m_pool, []() { return 1; });
    p.resolve();
    auto f = p.get_future();
    f.catched([](std::exception_ptr) {})
        .then([](int i) { return i; });

    f.get().apply([](int i) { ASSERT_EQ(i, 1); });
}

TEST_F(Future, CatchedMethodAppliedToRValue)
{
    auto p = make_promise(&m_pool, []() { return 2; });
    p.resolve();
    auto f = p.get_future()
        .then([](int) { return 3; })
        .catched([](std::exception_ptr) { FAIL(); });

    f.get().apply([](int i) { ASSERT_EQ(i, 3); });
}

TEST_F(Future, MayWorkOnThreadPoolWithoutMoveOnlyFunctorsSupport)
{
    thread_pool_with_functor_copy_requirement pool;
    auto p = make_promise(&pool, []() { return 2; });
    p.resolve();
    auto f = p.get_future()
        .then([](int i) { return i + 3; });

    f.get().apply([](int i) { ASSERT_EQ(i, 5); });
}

TEST_F(Future, DoNotExecuteChandedHandlersIfPreviousWasInterruptedByException)
{
    bool flag = false;
    event sync_event;
    auto p = make_promise(&m_pool, []() { return 2; });
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
    auto p = make_promise(&m_pool, []() { return 2; });
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
    auto p = make_promise(&m_pool, []() { return 2; });
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

TEST_F(Future, MethodGetThrowsExceptionIfExecutingTaskThrows)
{
    auto p = make_promise(&m_pool, []() -> int { throw std::runtime_error("message"); });
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
    auto p = make_promise(&m_pool, []() -> int { return 0; });
    auto f = p.get_future();
    p.resolve();
    f.get();

    event sync_event;
    f.then([&](int) { sync_event.set(); return 0; });

    sync_event.wait();
}

TEST_F(Future, ExecuteErrorHandlerIfThisHandlerSetAfterTaskExecution)
{
    auto p = make_promise(&m_pool, []() -> int { throw std::runtime_error("message"); });
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
    auto p = make_promise(&m_pool, []() -> int { return 0; });
    p.resolve();
    auto f = p.get_future();

    f.then([](int) { return 0; });
    EXPECT_ANY_THROW(f.then([](int) { return 2; }));
}

TEST_F(Future, CannotSetFailHandlerTwice)
{
    auto p = make_promise(&m_pool, []() -> int { return 0; });
    p.resolve();
    auto f = p.get_future();

    f.catched([](std::exception_ptr) {});
    EXPECT_ANY_THROW(f.catched([](std::exception_ptr) {}));
}

TEST_F(Future, CannotSetFailHandlerAfterSetSuccessHandler)
{
    auto p = make_promise(&m_pool, []() -> int { return 0; });
    p.resolve();
    auto f = p.get_future();

    f.then([](int) { return 0; });
    EXPECT_ANY_THROW(f.catched([](std::exception_ptr) {}));
}

TEST_F(Future, SuccessHandlerExecutesIfCorrespondingFutureAndPromiseDestroyed)
{
    event sync_event1;
    event sync_event2;
    auto p = std::make_unique<promise<thread_pool, int>>
                                  (make_promise(&m_pool, [&]() -> int { sync_event1.wait(); return 0; }));
    p->resolve();
    p->get_future()
        .then([&](int) { sync_event2.set(); return 0; });
    p.reset();

    sync_event1.set();
    sync_event2.wait();
}

TEST_F(Future, FailHandlerExecutesIfCorrespondingFutureAndPromiseDestroyed)
{
    event sync_event1;
    event sync_event2;
    auto p = std::make_unique<promise<thread_pool, int>>
        (make_promise(&m_pool, [&]() -> int { sync_event1.wait(); throw std::runtime_error(""); }));
    p->resolve();
    p->get_future()
        .catched([&](std::exception_ptr) { sync_event2.set();});
    p.reset();

    sync_event1.set();
    sync_event2.wait();
}

TEST_F(Future, PromiseFunctionNeverCopyIfMovedToPromiseConstructor)
{
    auto c = counter{};
    auto promise_task = [c = std::move(c)]() -> int {
        return 0;
    };
    auto p = make_promise(&m_pool, std::move(promise_task));
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
    auto p = make_promise(&m_pool, promise_task);
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
    auto p = make_promise(&m_pool, []() { return 1; });
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
    auto p = make_promise(&m_pool, []() { return 0; });
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

TEST_F(Future, FailHandlerNeverCopyIfMovedToCatchedMethod)
{
    auto c = counter{};
    auto task = [c = std::move(c)](std::exception_ptr){
    };
    auto p = make_promise(&m_pool, []() { return 1; });
    p.resolve();
    auto f = p.get_future()
        .catched(std::move(task));

    f.get();

    EXPECT_EQ(counter::copy_num, 0u);
    EXPECT_EQ(counter::copy_assign_num, 0u);
    EXPECT_EQ(counter::move_num, 2u);
    EXPECT_EQ(counter::move_assign_num, 0u);
}

TEST_F(Future, FutureDataMayApplyHandlerWithOneParamerterWithVariousQualificators)
{
    static volatile int vi = 1;
    auto p1 = make_promise(&m_pool, []()->int { return 1; });
    auto p2 = make_promise(&m_pool, []()->volatile int &{ return vi; });
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

    auto p1 = make_promise(&m_pool, []()->int { return 1; });
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
    auto p = make_promise(&m_pool, []()->std::string { return "data"; });
    p.resolve();
    auto f = p.get_future();

    auto data = f.get();
    data.apply([](std::string i) { EXPECT_EQ(i, "data"); i[0] = 's'; });
    data.apply([](const std::string i) { EXPECT_EQ(i, "data"); });

    data.apply([](const std::string &i) { EXPECT_EQ(i, "data"); });
}

TEST_F(Future, FutureDataMakeAcceptRValueRefParameteraziedFunctorAndMoveValue)
{
    auto p = make_promise(&m_pool, []()->std::string { return "data"; });
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
    auto p = make_promise(&m_pool, []()->std::string { return "data"; });
    p.resolve();
    auto f1 = p.get_future();
    auto data1 = f1.get();
    data1.apply([&](std::string &i) { i[0] = 's'; });

    data1.apply([&](const std::string &i) { EXPECT_EQ(i, "sata"); });
}

TEST_F(Future, ConstFutureDataMakeCopyOfValueIfFunctorHasNonReferenceParameter)
{
    auto p = make_promise(&m_pool, []()->std::string { return "data"; });
    p.resolve();
    auto f = p.get_future();

    const auto data = f.get();
    data.apply([](std::string i) { EXPECT_EQ(i, "data"); });
    data.apply([](const std::string i) { EXPECT_EQ(i, "data"); });

    data.apply([](const std::string &i) { EXPECT_EQ(i, "data"); });
}

TEST_F(Future, PassMoveOnlyTypeThroughChainHanlers)
{
    auto p = make_promise(&m_pool, []()->std::unique_ptr<int> { return std::make_unique<int>(3); });
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

    auto p1 = make_promise(&m_pool, []()->int { return 1; });
    p1.resolve();
    auto p2 = make_promise(&m_pool, []()->int &{ return i2; });
    p2.resolve();
    auto p3 = make_promise(&m_pool, []()->int &&{ return std::move(i3); });
    p3.resolve();
    auto p4 = make_promise(&m_pool, []()->const int { return 4; });
    p4.resolve();
    auto p5 = make_promise(&m_pool, []()->const int &{ return i5; });
    p5.resolve();
    auto p6 = make_promise(&m_pool, []()->const int &&{ return std::move(i6); });
    p6.resolve();
    auto p7 = make_promise(&m_pool, []()->volatile int { return 7; });
    p7.resolve();
    auto p8 = make_promise(&m_pool, []()->volatile int &{ return i8; });
    p8.resolve();
    auto p9 = make_promise(&m_pool, []()->volatile int &&{ return std::move(i9); });
    p9.resolve();
    auto p10 = make_promise(&m_pool, []()->const volatile int { return 10; });
    p10.resolve();
    auto p11 = make_promise(&m_pool, []()->const volatile int &{ return i11; });
    p11.resolve();
    auto p12 = make_promise(&m_pool, []()->const volatile int &&{ return std::move(i12); });
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
    auto p = make_promise(&m_pool, []() { return 1; });
    p.get_future();
    ASSERT_ANY_THROW(p.get_future());
}

TEST_F(Future, PromiseCannotExecuteResolveTwice)
{
    auto p = make_promise(&m_pool, []() { return 1; });
    p.resolve();
    ASSERT_ANY_THROW(p.resolve());
}

TEST_F(Future, DoesNotCopyMovableObjectInInnerStorage)
{
    auto p = make_promise(&m_pool, []() { return counter{}; });
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
    auto p = make_promise(&m_pool, []() {});
    p.resolve();
    auto f = p.get_future()
                .then([](){});

    f.get();
    static_assert(std::is_void_v<decltype(f.get())>);
}

TEST_F(Future, DoChainingWithVoidReturnCallback)
{
    int i = 0;
    auto p = make_promise(&m_pool, []() {});
    p.resolve();
    auto f = p.get_future()
        .then([]() {})
        .then([]() {})
        .then([]() { return 1; })
        .then([](int) {})
        .then([]() {return 22; })
        .then([&i](int ii) { i = ii; });

    f.get();
    ASSERT_EQ(i, 22);
}

TEST_F(Future, PromiseAcceptParametersToResolve)
{
    int i = 0;
    double d = 0;
    auto p = make_promise(&m_pool, [&](int ii, double dd) { i = ii; d = dd; });
    p.resolve(1, 9);
    p.get_future().get();

    EXPECT_EQ(i, 1);
    EXPECT_EQ(d, 9);
}

TEST_F(Future, PromiseMovesParametersToResolve)
{
    int r = 0;
    auto i = std::make_unique<int>(2);
    counter c;
    auto p = make_promise(&m_pool, [&](std::unique_ptr<int> ii, counter &&) { r = *ii; });
    p.resolve(std::move(i), std::move(c));
    p.get_future().get();

    EXPECT_FALSE(i);
    EXPECT_EQ(r, 2);
    EXPECT_EQ(counter::copy_num, 0u);
    EXPECT_EQ(counter::copy_assign_num, 0u);
    EXPECT_TRUE(counter::move_num > 0u);
    EXPECT_EQ(counter::move_assign_num, 0u);
}

TEST_F(Future, PromiseCopiesParametersToResolve)
{
    counter c;
    auto p = make_promise(&m_pool, [](const counter &) { return 1; });
    p.resolve(c);
    p.get_future().get();

    EXPECT_EQ(counter::copy_num, 1u);
    EXPECT_EQ(counter::copy_assign_num, 0u);
    EXPECT_TRUE(counter::move_num > 0u);
    EXPECT_EQ(counter::move_assign_num, 0u);
}

TEST_F(Future, InvalidOnDefaultConstruction)
{
    future<thread_pool, int> f;

    ASSERT_FALSE(f.is_valid());
}

TEST_F(Future, InvalidAfterMove)
{
    auto p = make_promise(&m_pool, []() {});
    auto f = p.get_future();

    auto f2(std::move(f));

    EXPECT_TRUE(f2.is_valid());
    EXPECT_FALSE(f.is_valid());
}

TEST_F(Future, InvalidAfterMoveAssign)
{
    auto p1 = make_promise(&m_pool, []() {});
    auto f1 = p1.get_future();
    auto p2 = make_promise(&m_pool, []() {});
    auto f2 = p2.get_future();

    f2 = std::move(f1);

    EXPECT_TRUE(f2.is_valid());
    EXPECT_FALSE(f1.is_valid());
}

TEST_F(Future, ActuallyDoesMove)
{
    auto p = make_promise(&m_pool, []() { return 1; });
    auto f = p.get_future();
    p.resolve();

    auto f2(std::move(f));
    f2.get().apply([](int i) { ASSERT_EQ(i, 1); });

    EXPECT_TRUE(f2.is_valid());
    EXPECT_FALSE(f.is_valid());
}

TEST_F(Future, ActuallyDoesMoveAssign)
{
    auto p = make_promise(&m_pool, []() { return 1; });
    auto f = p.get_future();
    p.resolve();

    future<thread_pool, int> f2;
    f2 = std::move(f);
    f2.get().apply([](int i) { ASSERT_EQ(i, 1); });

    EXPECT_TRUE(f2.is_valid());
    EXPECT_FALSE(f.is_valid());
}

TEST_F(Future, IsValidIfPromiseDestroyed)
{
    auto p = std::make_unique<promise<thread_pool, int>>(make_promise(&m_pool, []() { return 1; }));
    auto f = p->get_future();
    p->resolve();
    p.reset();

    ASSERT_TRUE(f.is_valid());
    f.get().apply([](int i) { ASSERT_EQ(i, 1); });
}

TEST_F(Future, ThenAcceptParameterWhichPromiseFunctionReturns)
{
    int i = 0;
    auto p0 = make_promise(&m_pool, [&]() -> void {}); p0.resolve();
    auto f0 = p0.get_future().then([&]() {}); f0;
    auto p1 = make_promise(&m_pool, [&]() -> int { return i; }); p1.resolve();
    auto f1 = p1.get_future().then([&](int v) { EXPECT_EQ(v, i); }); f1;
    auto p2 = make_promise(&m_pool, [&]() -> int &{ return i; }); p2.resolve();
    auto f2 = p2.get_future().then([&](int &v) {  EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); }); f2;
    auto p3 = make_promise(&m_pool, [&]() -> int &&{ return std::move(i); }); p3.resolve();
    auto f3 = p3.get_future().then([&](int &&v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); }); f3;
    auto p4 = make_promise(&m_pool, [&]() -> const int { return i; }); p4.resolve();
    auto f4 = p4.get_future().then([&](const int v) { EXPECT_EQ(v, i); }); f4;
    auto p5 = make_promise(&m_pool, [&]() -> const int &{ return i; }); p5.resolve();
    auto f5 = p5.get_future().then([&](const int &v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); }); f5;
    auto p6 = make_promise(&m_pool, [&]() -> const int &&{ return std::move(i); }); p6.resolve();
    auto f6 = p6.get_future().then([&](const int &&v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); }); f6;
    auto p7 = make_promise(&m_pool, [&]() -> volatile int { return i; }); p7.resolve();
    auto f7 = p7.get_future().then([&](volatile int v) { EXPECT_EQ(v, i); }); f7;
    auto p8 = make_promise(&m_pool, [&]() -> volatile int &{ return i; }); p8.resolve();
    auto f8 = p8.get_future().then([&](volatile int &v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); }); f8;
    auto p9 = make_promise(&m_pool, [&]() -> volatile int &&{ return std::move(i); }); p9.resolve();
    auto f9 = p9.get_future().then([&](volatile int &&v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); }); f9;
    auto p10 = make_promise(&m_pool, [&]() -> const volatile int { return i; }); p10.resolve();
    auto f10 = p10.get_future().then([&](const volatile int v) { EXPECT_EQ(v, i); }); f10;
    auto p11 = make_promise(&m_pool, [&]() -> const volatile int &{ return i; }); p11.resolve();
    auto f11 = p11.get_future().then([&](const volatile int &v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); }); f11;
    auto p12 = make_promise(&m_pool, [&]() -> const volatile int &&{ return std::move(i); }); p12.resolve();
    auto f12 = p12.get_future().then([&](const volatile int &&v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); }); f12;

    f0.get();
    f1.get();
    f2.get();
    f3.get();
    f4.get();
    f5.get();
    f6.get();
    f7.get();
    f8.get();
    f9.get();
    f10.get();
    f11.get();
    f12.get();
}

TEST_F(Future, ThenAcceptParameterWhichPromiseFunctionReturnsAndPassIfFuther)
{
    int i = 0;
    auto p1 = make_promise(&m_pool, [&]() ->int { return i; }); p1.resolve();
    auto f1 = p1.get_future().then([&](int v) ->decltype(auto) {  return v; }); f1;
    auto p2 = make_promise(&m_pool, [&]() -> int &{ return i; }); p2.resolve();
    auto f2 = p2.get_future().then([&](int &v) ->decltype(auto) { return v; }); f2;
    auto p3 = make_promise(&m_pool, [&]() -> int &&{ return std::move(i); }); p3.resolve();
    auto f3 = p3.get_future().then([&](int &&v) ->decltype(auto) {  return std::move(v); }); f3;
    auto p4 = make_promise(&m_pool, [&]() -> const int { return i; }); p4.resolve();
    auto f4 = p4.get_future().then([&](const int v) ->decltype(auto) {  return v; }); f4;
    auto p5 = make_promise(&m_pool, [&]() -> const int &{ return i; }); p5.resolve();
    auto f5 = p5.get_future().then([&](const int &v) ->decltype(auto) {  return v; }); f5;
    auto p6 = make_promise(&m_pool, [&]() -> const int &&{ return std::move(i); }); p6.resolve();
    auto f6 = p6.get_future().then([&](const int &&v) ->decltype(auto) {  return std::move(v); }); f6;
    auto p7 = make_promise(&m_pool, [&]() -> volatile int { return i; }); p7.resolve();
    auto f7 = p7.get_future().then([&](volatile int v) ->decltype(auto) { return v; }); f7;
    auto p8 = make_promise(&m_pool, [&]() -> volatile int &{ return i; }); p8.resolve();
    auto f8 = p8.get_future().then([&](volatile int &v) ->decltype(auto) { return v; }); f8;
    auto p9 = make_promise(&m_pool, [&]() -> volatile int &&{ return std::move(i); }); p9.resolve();
    auto f9 = p9.get_future().then([&](volatile int &&v) ->decltype(auto) { return std::move(v); }); f9;
    auto p10 = make_promise(&m_pool, [&]() -> const volatile int { return i; }); p10.resolve();
    auto f10 = p10.get_future().then([&](const volatile int v) ->decltype(auto) { return v; }); f10;
    auto p11 = make_promise(&m_pool, [&]() -> const volatile int &{ return i; }); p11.resolve();
    auto f11 = p11.get_future().then([&](const volatile int &v) ->decltype(auto) { return v; }); f11;
    auto p12 = make_promise(&m_pool, [&]() -> const volatile int &&{ return std::move(i); }); p12.resolve();
    auto f12 = p12.get_future().then([&](const volatile int &&v) ->decltype(auto) { return std::move(v); }); f12;

    f1.get().apply([&](int v) { EXPECT_EQ(v, i); });
    f2.get().apply([&](int &v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); });
    f3.get().apply([&](int &&v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); });
    f4.get().apply([&](const int v) { EXPECT_EQ(v, i); });
    f5.get().apply([&](const int &v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); });
    f6.get().apply([&](const int &&v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); });
    f7.get().apply([&](volatile int v) { EXPECT_EQ(v, i); });
    f8.get().apply([&](volatile int &v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); });
    f9.get().apply([&](volatile int &&v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); });
    f10.get().apply([&](const volatile int v) { EXPECT_EQ(v, i); });
    f11.get().apply([&](const volatile int &v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); });
    f12.get().apply([&](const volatile int &&v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); });
}

TEST_F(Future, ThenFunctionMayReferenceParameterIfFutureStoreValue)
{
    int *a1 = nullptr;
    auto p1 = make_promise(&m_pool, [&]() { return 1; }); p1.resolve();
    auto f1 = p1.get_future();
    f1.then([&](int &v) { a1 = &v; v = 2; });
    int *a2 = nullptr;
    auto p2 = make_promise(&m_pool, [&]() { return 1; }); p2.resolve();
    auto f2 = p2.get_future();
    f2.then([&](int &&v) { a2 = &v; v = 2; });
    const int *a3 = nullptr;
    auto p3 = make_promise(&m_pool, [&]() { return 1; }); p3.resolve();
    auto f3 = p3.get_future();
    f3.then([&](const int &v) { a3 = &v; });
    const int *a4 = nullptr;
    auto p4 = make_promise(&m_pool, [&]() { return 1; }); p4.resolve();
    auto f4 = p4.get_future();
    f4.then([&](const int &&v) { a4 = &v; });
    volatile int *a5 = nullptr;
    auto p5 = make_promise(&m_pool, [&]() { return 1; }); p5.resolve();
    auto f5 = p5.get_future();
    f5.then([&](volatile int &v) { a5 = &v; v = 2; });
    volatile int *a6 = nullptr;
    auto p6 = make_promise(&m_pool, [&]() { return 1; }); p6.resolve();
    auto f6 = p6.get_future();
    f6.then([&](volatile int &&v) { a6 = &v; v = 2; });
    const volatile int *a7 = nullptr;
    auto p7 = make_promise(&m_pool, [&]() { return 1; }); p7.resolve();
    auto f7 = p7.get_future();
    f7.then([&](const volatile int &v) { a7 = &v; });
    const volatile int *a8 = nullptr;
    auto p8 = make_promise(&m_pool, [&]() { return 1; }); p8.resolve();
    auto f8 = p8.get_future();
    f8.then([&](const volatile int &&v) { a8 = &v; });

    f1.get().apply([&](int &v) { EXPECT_EQ(v, 2); EXPECT_EQ(a1, &v); });
    f2.get().apply([&](int &v) { EXPECT_EQ(v, 2); EXPECT_EQ(a2, &v); });
    f3.get().apply([&](int &v) { EXPECT_EQ(v, 1); EXPECT_EQ(a3, &v); });
    f4.get().apply([&](int &v) { EXPECT_EQ(v, 1); EXPECT_EQ(a4, &v); });
    f5.get().apply([&](int &v) { EXPECT_EQ(v, 2); EXPECT_EQ(a5, &v); });
    f6.get().apply([&](int &v) { EXPECT_EQ(v, 2); EXPECT_EQ(a6, &v); });
    f7.get().apply([&](int &v) { EXPECT_EQ(v, 1); EXPECT_EQ(a7, &v); });
    f8.get().apply([&](int &v) { EXPECT_EQ(v, 1); EXPECT_EQ(a8, &v); });
}


TEST_F(Future, MoveOnlyObjectMayBePassedToSuccessCallbackByRValueRef)
{
    std::unique_ptr<int> v;
    auto p1 = make_promise(&m_pool, [&]() { return std::make_unique<int>(2); });
    p1.resolve();
    p1.get_future()
        .then([&](std::unique_ptr<int>&& i) { return std::move(i); })
        .then([&](std::unique_ptr<int>&& i) { v = std::move(i); })
        .get();


    ASSERT_TRUE(v);
    ASSERT_EQ(*v, 2);
}

TEST_F(Future, MoveOnlyObjectMayBePassedThroughChainAndAchievedViaGet)
{

    std::unique_ptr<int> v;
    auto p1 = make_promise(&m_pool, [&]() { return std::make_unique<int>(2); });
    p1.resolve();
    p1.get_future()
        .then([&](std::unique_ptr<int> &&i) { return std::move(i); })
        .then([&](std::unique_ptr<int> &&i) { return std::move(i); })
        .get()
        .apply([&](std::unique_ptr<int> &&i) { v = std::move(i); });

    ASSERT_TRUE(v);
    ASSERT_EQ(*v, 2);
}

TEST_F(Future, DoNotExecuteTheRestOfHandlerIfExceptionHappened)
{
    auto p = make_promise(&m_pool, []() {});
    p.resolve();
    p.get_future()
        .then([]() {})
        .then([]() { throw std::runtime_error(""); })
        .then([]() { FAIL(); });
}

TEST_F(Future, FutureDataFunctionParameterMayBeValue)
{
    auto p = make_promise(&m_pool, []() { return 1; });
    p.resolve();

    p.get_future().get().apply([](int i) { EXPECT_EQ(i, 1); });
}

TEST_F(Future, FutureDataFunctionParameterMayBeNonConstReferenceForConstFutureAndData)
{
    auto p = make_promise(&m_pool, []() { return 1; });
    p.resolve();

    const auto f = p.get_future();
    const auto d = f.get();
    d.apply([](int &i) { i = 4; });

    d.apply([](const int &i) { EXPECT_EQ(i, 4); });
}

TEST_F(Future, IfSuccessCallbackReturnsFutureThenValueFromItPassesToFutherFuture)
{
    auto p = make_promise(&m_pool, []() { return 1; });
    p.resolve();

    auto f = p.get_future().then([&](int i) {
        auto p1 = make_promise(&m_pool, [](int ii) { return ii * 2; });
        p1.resolve(i);
        return p1.get_future();
    });

    static_assert(std::is_same_v<decltype(f), future<thread_pool, int>>);

    f.get().apply([](int i) { ASSERT_EQ(i, 2); });
}

TEST_F(Future, IfSuccessCallbackReturnsFutureThenAnyTypeValueFromItPassesToFutherFuture)
{
    bool f0_completed = false;
    int i = 1;
    volatile int ii = 2;

    auto p0 = make_promise(&m_pool, []() {}); p0.resolve();
    auto f0 = p0.get_future().then([&]() {
        auto p = make_promise(&m_pool, [&]() { });
        p.resolve();
        return p.get_future();
    }).then([&]() { f0_completed = true; });

    auto p1 = make_promise(&m_pool, []() {}); p1.resolve();
    auto f1 = p1.get_future().then([&]() {
        auto p = make_promise(&m_pool, [&]() -> int { return i; });
        p.resolve();
        return p.get_future();
    });
    auto p2 = make_promise(&m_pool, []() {}); p2.resolve();
    auto f2 = p2.get_future().then([&]() {
        auto p = make_promise(&m_pool, [&]() -> int & { return i; });
        p.resolve();
        return p.get_future();
    });
    auto p3 = make_promise(&m_pool, []() {}); p3.resolve();
    auto f3 = p3.get_future().then([&]() {
        auto p = make_promise(&m_pool, [&]() -> int &&{ return std::move(i); });
        p.resolve();
        return p.get_future();
    });
    auto p4 = make_promise(&m_pool, []() {}); p4.resolve();
    auto f4 = p4.get_future().then([&]() {
        auto p = make_promise(&m_pool, [&]() -> const int { return i; });
        p.resolve();
        return p.get_future();
    });
    auto p5 = make_promise(&m_pool, []() {}); p5.resolve();
    auto f5 = p5.get_future().then([&]() {
        auto p = make_promise(&m_pool, [&]() -> const int &{ return i; });
        p.resolve();
        return p.get_future();
    });
    auto p6 = make_promise(&m_pool, []() {}); p6.resolve();
    auto f6 = p6.get_future().then([&]() {
        auto p = make_promise(&m_pool, [&]() -> const int &&{ return std::move(i); });
        p.resolve();
        return p.get_future();
    });
    auto p7 = make_promise(&m_pool, []() {}); p7.resolve();
    auto f7 = p7.get_future().then([&]() {
        auto p = make_promise(&m_pool, [&]() -> volatile int { return ii; });
        p.resolve();
        return p.get_future();
    });
    auto p8 = make_promise(&m_pool, []() {}); p8.resolve();
    auto f8 = p8.get_future().then([&]() {
        auto p = make_promise(&m_pool, [&]() -> volatile int &{ return ii; });
        p.resolve();
        return p.get_future();
    });
    auto p9 = make_promise(&m_pool, []() {}); p9.resolve();
    auto f9 = p9.get_future().then([&]() {
        auto p = make_promise(&m_pool, [&]() -> volatile int &&{ return std::move(ii); });
        p.resolve();
        return p.get_future();
    });
    auto p10 = make_promise(&m_pool, []() {}); p10.resolve();
    auto f10 = p10.get_future().then([&]() {
        auto p = make_promise(&m_pool, [&]() -> const volatile int { return ii; });
        p.resolve();
        return p.get_future();
    });
    auto p11 = make_promise(&m_pool, []() {}); p11.resolve();
    auto f11 = p11.get_future().then([&]() {
        auto p = make_promise(&m_pool, [&]() -> const volatile int &{ return ii; });
        p.resolve();
        return p.get_future();
    });
    auto p12 = make_promise(&m_pool, []() {}); p12.resolve();
    auto f12 = p12.get_future().then([&]() {
        auto p = make_promise(&m_pool, [&]() -> const volatile int &&{ return std::move(ii); });
        p.resolve();
        return p.get_future();
    });

    f0.get();
    ASSERT_TRUE(f0_completed);

    f1.get().apply([&](const int &v) { EXPECT_EQ(v, 1); });
    f2.get().apply([&](const int &v) { EXPECT_EQ(v, 1); EXPECT_EQ(&v, &i); });
    f3.get().apply([&](const int &v) { EXPECT_EQ(v, 1); EXPECT_EQ(&v, &i); });
    f4.get().apply([&](const int &v) { EXPECT_EQ(v, 1); });
    f5.get().apply([&](const int &v) { EXPECT_EQ(v, 1); EXPECT_EQ(&v, &i); });
    f6.get().apply([&](const int &v) { EXPECT_EQ(v, 1); EXPECT_EQ(&v, &i); });
    f7.get().apply([&](const volatile int &v) { EXPECT_EQ(v, 2); });
    f8.get().apply([&](const volatile int &v) { EXPECT_EQ(v, 2); EXPECT_EQ(&v, &ii); });
    f9.get().apply([&](const volatile int &v) { EXPECT_EQ(v, 2); EXPECT_EQ(&v, &ii); });
    f10.get().apply([&](const volatile int &v) { EXPECT_EQ(v, 2); });
    f11.get().apply([&](const volatile int &v) { EXPECT_EQ(v, 2); EXPECT_EQ(&v, &ii); });
    f12.get().apply([&](const volatile int &v) { EXPECT_EQ(v, 2); EXPECT_EQ(&v, &ii); });
}

TEST_F(Future, SuccessCallbackReturnsFutureWithMoveOnlyType)
{
    auto p = make_promise(&m_pool, []() { return std::make_unique<int>(2); }); p.resolve();
    auto f = p.get_future()
     .then([&](std::unique_ptr<int> &&ptr) {
        auto p = make_promise(&m_pool, [ptr = std::move(ptr)]() mutable { return std::move(ptr); });
        p.resolve();
        return p.get_future(); })
     .then([](std::unique_ptr<int> &&ptr) mutable {
        EXPECT_TRUE(ptr);
        EXPECT_EQ(*ptr, 2);
        return std::move(ptr);
     });

    f.get().apply([](std::unique_ptr<int> &&ptr) { ASSERT_TRUE(ptr); ASSERT_EQ(*ptr, 2); });
}

TEST_F(Future, SuccessCallbackReturnsFutureAndCopiesStoredVarible)
{
    auto p = make_promise(&m_pool, []() { return std::string("test"); }); p.resolve();
    auto f = p.get_future()
        .then([&](std::string &&str) {
               auto p = make_promise(&m_pool, [str = std::move(str)]() mutable { return str; });
               p.resolve();
               return p.get_future(); })
        .then([](std::string ptr) mutable {
                  EXPECT_EQ(ptr, "test");
                  return ptr;
              });

    f.get().apply([](std::string str) { ASSERT_EQ(str, "test"); });
}

TEST_F(Future, IfSuccessCallbackReturnsFutureThenHappenedExceptionGoesThroughAllChain)
{
    event sync_event1;
    auto p1 = make_promise(&m_pool, []() { throw std::runtime_error("message"); }); p1.resolve();
    auto f1 = p1.get_future()
        .then([&]() {
            auto p = make_promise(&m_pool, [&]() {});
            p.resolve();
            return p.get_future();
        })
        .then([]() { FAIL(); })
        .catched([&](std::exception_ptr) { sync_event1.set(); });

    sync_event1.wait();
    try {
        f1.get();
        FAIL();
    } catch (const std::runtime_error &e) {
        EXPECT_EQ(std::string(e.what()), "message");
    }


    event sync_event2;
    auto p2 = make_promise(&m_pool, []() {}); p2.resolve();
    auto f2 = p2.get_future()
        .then([]() { throw std::runtime_error("message"); })
        .then([&]() {
             auto p = make_promise(&m_pool, [&]() {});
             p.resolve();
             return p.get_future();
         })
        .then([]() { FAIL(); })
        .catched([&](std::exception_ptr) { sync_event2.set(); });

    sync_event2.wait();
    try {
        f2.get();
        FAIL();
    }
    catch(const std::runtime_error &e) {
        EXPECT_EQ(std::string(e.what()), "message");
    }


    event sync_event3;
    auto p3 = make_promise(&m_pool, []() {}); p3.resolve();
    auto f3 = p3.get_future()
        .then([]() { })
        .then([&]() {
             auto p = make_promise(&m_pool, [&]() { throw std::runtime_error("message"); });
             p.resolve();
             return p.get_future();
         })
        .then([]() { FAIL(); })
        .catched([&](std::exception_ptr) { sync_event3.set(); });

    sync_event3.wait();
    try {
        f3.get();
        FAIL();
    } catch(const std::runtime_error &e) {
        EXPECT_EQ(std::string(e.what()), "message");
    }
}

TEST_F(Future, PromiseResolveFunctionReturnsFuture)
{
    bool f0_completed = false;
    const auto master_thread_id = std::this_thread::get_id();
    int i = 1;
    volatile int ii = 2;

    auto p0 = make_promise(&m_pool, [&](int) {
        EXPECT_EQ(master_thread_id, std::this_thread::get_id());
        auto p = make_promise(&m_pool, [&]() {}); p.resolve();
        return p.get_future();
    }); p0.resolve(6);
    auto f0 = p0.get_future()
        .then([&]() { f0_completed = true; });
    auto p1 = make_promise(&m_pool, [&](int) {
        EXPECT_EQ(master_thread_id, std::this_thread::get_id());
        auto p = make_promise(&m_pool, [&]() -> int { return i; }); p.resolve();
        return p.get_future();
    }); p1.resolve(6);
    auto f1 = p1.get_future()
        .then([&](int v) -> decltype(auto) { EXPECT_EQ(v, i); return v; });
    auto p2 = make_promise(&m_pool, [&](int) {
        EXPECT_EQ(master_thread_id, std::this_thread::get_id());
        auto p = make_promise(&m_pool, [&]() -> int &{ return i; }); p.resolve();
        return p.get_future();
    }); p2.resolve(6);
    auto f2 = p2.get_future()
        .then([&](int &v) -> decltype(auto) { EXPECT_EQ(v, i); return v; });
    auto p3 = make_promise(&m_pool, [&](int) {
        EXPECT_EQ(master_thread_id, std::this_thread::get_id());
        auto p = make_promise(&m_pool, [&]() -> int &&{ return std::move(i); }); p.resolve();
        return p.get_future();
    }); p3.resolve(6);
    auto f3 = p3.get_future()
        .then([&](int &&v) -> decltype(auto) { EXPECT_EQ(v, i); return std::move(v); });
    auto p4 = make_promise(&m_pool, [&](int) {
        EXPECT_EQ(master_thread_id, std::this_thread::get_id());
        auto p = make_promise(&m_pool, [&]() -> const int { return i; }); p.resolve();
        return p.get_future();
    }); p4.resolve(6);
    auto f4 = p4.get_future()
        .then([&](const int v) -> decltype(auto) { EXPECT_EQ(v, i); return v; });
    auto p5 = make_promise(&m_pool, [&](int) {
        EXPECT_EQ(master_thread_id, std::this_thread::get_id());
        auto p = make_promise(&m_pool, [&]() -> const int &{ return i; }); p.resolve();
        return p.get_future();
    }); p5.resolve(6);
    auto f5 = p5.get_future()
        .then([&](const int &v) -> decltype(auto) { EXPECT_EQ(v, i); return v; });
    auto p6 = make_promise(&m_pool, [&](int) {
        EXPECT_EQ(master_thread_id, std::this_thread::get_id());
        auto p = make_promise(&m_pool, [&]() -> const int &&{ return std::move(i); }); p.resolve();
        return p.get_future();
    }); p6.resolve(6);
    auto f6 = p6.get_future()
        .then([&](const int &&v) -> decltype(auto) { EXPECT_EQ(v, i); return std::move(v); });
    auto p7 = make_promise(&m_pool, [&](int) {
        EXPECT_EQ(master_thread_id, std::this_thread::get_id());
        auto p = make_promise(&m_pool, [&]() -> volatile int { return ii; }); p.resolve();
        return p.get_future();
    }); p7.resolve(6);
    auto f7 = p7.get_future()
        .then([&](volatile int v) -> decltype(auto) { EXPECT_EQ(v, ii); return v; });
    auto p8 = make_promise(&m_pool, [&](int) {
        EXPECT_EQ(master_thread_id, std::this_thread::get_id());
        auto p = make_promise(&m_pool, [&]() -> volatile int &{ return ii; }); p.resolve();
        return p.get_future();
    }); p8.resolve(6);
    auto f8 = p8.get_future()
        .then([&](volatile int &v) -> decltype(auto) { EXPECT_EQ(v, ii); return v; });
    auto p9 = make_promise(&m_pool, [&](int) {
        EXPECT_EQ(master_thread_id, std::this_thread::get_id());
        auto p = make_promise(&m_pool, [&]() -> volatile int &&{ return std::move(ii); }); p.resolve();
        return p.get_future();
    }); p9.resolve(6);
    auto f9 = p9.get_future()
        .then([&](volatile int &&v) -> decltype(auto) { EXPECT_EQ(v, ii); return std::move(v); });
    auto p10 = make_promise(&m_pool, [&](int) {
        EXPECT_EQ(master_thread_id, std::this_thread::get_id());
        auto p = make_promise(&m_pool, [&]() -> const volatile int { return ii; }); p.resolve();
        return p.get_future();
    }); p10.resolve(6);
    auto f10 = p10.get_future()
        .then([&](const volatile int v) -> decltype(auto) { EXPECT_EQ(v, ii); return v; });
    auto p11 = make_promise(&m_pool, [&](int) {
        EXPECT_EQ(master_thread_id, std::this_thread::get_id());
        auto p = make_promise(&m_pool, [&]() -> const volatile int &{ return ii; }); p.resolve();
        return p.get_future();
    }); p11.resolve(6);
    auto f11 = p11.get_future()
        .then([&](const volatile int &v) -> decltype(auto) { EXPECT_EQ(v, ii); return v; });
    auto p12 = make_promise(&m_pool, [&](int) {
        EXPECT_EQ(master_thread_id, std::this_thread::get_id());
        auto p = make_promise(&m_pool, [&]() -> const volatile int &&{ return std::move(ii); }); p.resolve();
        return p.get_future();
    }); p12.resolve(6);
    auto f12 = p12.get_future()
        .then([&](const volatile int &&v) -> decltype(auto) { EXPECT_EQ(v, ii); return std::move(v); });


    EXPECT_ANY_THROW(p0.resolve(8));
    EXPECT_ANY_THROW(p1.resolve(8));
    EXPECT_ANY_THROW(p2.resolve(8));
    EXPECT_ANY_THROW(p3.resolve(8));
    EXPECT_ANY_THROW(p4.resolve(8));
    EXPECT_ANY_THROW(p5.resolve(8));
    EXPECT_ANY_THROW(p6.resolve(8));
    EXPECT_ANY_THROW(p7.resolve(8));
    EXPECT_ANY_THROW(p8.resolve(8));
    EXPECT_ANY_THROW(p9.resolve(8));
    EXPECT_ANY_THROW(p10.resolve(8));
    EXPECT_ANY_THROW(p11.resolve(8));
    EXPECT_ANY_THROW(p12.resolve(8));

    f0.get();
    ASSERT_TRUE(f0_completed);
    f1.get().apply([&](const int &v) { EXPECT_EQ(v, i); });
    f2.get().apply([&](const int &v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); });
    f3.get().apply([&](const int &v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); });
    f4.get().apply([&](const int &v) { EXPECT_EQ(v, i); });
    f5.get().apply([&](const int &v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); });
    f6.get().apply([&](const int &v) { EXPECT_EQ(v, i); EXPECT_EQ(&v, &i); });
    f7.get().apply([&](const volatile int &v) { EXPECT_EQ(v, ii); });
    f8.get().apply([&](const volatile int &v) { EXPECT_EQ(v, ii); EXPECT_EQ(&v, &ii); });
    f9.get().apply([&](const volatile int &v) { EXPECT_EQ(v, ii); EXPECT_EQ(&v, &ii); });
    f10.get().apply([&](const volatile int &v) { EXPECT_EQ(v, ii); });
    f11.get().apply([&](const volatile int &v) { EXPECT_EQ(v, ii); EXPECT_EQ(&v, &ii); });
    f12.get().apply([&](const volatile int &v) { EXPECT_EQ(v, ii); EXPECT_EQ(&v, &ii); });
}

TEST_F(Future, PromiseResolveFunctionReturnsFutureWhichThrowsException)
{
    event sync_event1;
    auto p1 = make_promise(&m_pool, [&](int) -> future<thread_pool, int> { throw std::runtime_error("test"); });
    p1.resolve(6);
    auto f1 = p1.get_future()
        .then([](int) { FAIL(); })
        .catched([&](std::exception_ptr) { sync_event1.set(); });

    sync_event1.wait();
    try {
        f1.get();
        FAIL();
    } catch (const std::runtime_error &e) {
        EXPECT_EQ(std::string(e.what()), "test");
    }

    event sync_event2;
    auto p2 = make_promise(&m_pool, [&](int) {
                               auto p = make_promise(&m_pool, [&]() -> int {
                                   throw std::runtime_error("test");
                               });
                               p.resolve();
                               return p.get_future();
                           });
    p2.resolve(6);
    auto f2 = p2.get_future()
        .then([](int) { FAIL(); })
        .catched([&](std::exception_ptr) { sync_event2.set(); });

    sync_event2.wait();
    try {
        f2.get();
        FAIL();
    } catch(const std::runtime_error &e) {
        EXPECT_EQ(std::string(e.what()), "test");
    }

}

TEST_F(Future, PromiseFunctionWhichReturnsFutureAcceptsMoveOnlyTypeAndPassesItFuther)
{
    auto p = make_promise(&m_pool, [&](std::unique_ptr<int> ptr) {
        auto p = make_promise(&m_pool, [ptr = std::move(ptr)]() mutable { return std::move(ptr); });
        p.resolve();
        return p.get_future();
    });
    p.resolve(std::make_unique<int>(3));
    auto f = p.get_future()
        .then([](std::unique_ptr<int> &&ptr) { return std::move(ptr); });

    f.get().apply([](std::unique_ptr<int> &&ptr) { ASSERT_TRUE(ptr); ASSERT_EQ(*ptr, 3); });
}

TEST_F(Future, PromiseFunctionWhichReturnsFutureCopiesVarible)
{
    std::string s = "test";
    auto p = make_promise(&m_pool, [&](std::string str) {
        auto p = make_promise(&m_pool, [str]() { return str; });
        p.resolve();
        return p.get_future();
    });
    p.resolve(s);
    auto f = p.get_future()
        .then([&](std::string &&str) {
              auto p = make_promise(&m_pool, [str = std::move(str)]() mutable { return str; });
              p.resolve();
              return p.get_future();
         })
        .then([](const std::string &ptr) mutable {
             EXPECT_EQ(ptr, "test");
             return ptr;
         });

    f.get().apply([](std::string str) { ASSERT_EQ(str, "test"); });
}
