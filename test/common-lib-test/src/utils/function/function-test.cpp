#include <common-lib/utils/function.h>

#include <gtest/gtest.h>

#include <array>

using namespace vshalygin::cl;
using namespace testing;

static_assert(!std::is_copy_constructible_v<function<void()>>);
static_assert(!std::is_copy_assignable_v<function<void()>>);
static_assert(std::is_move_constructible_v<function<void()>>);
static_assert(std::is_move_assignable_v<function<void()>>);

namespace {
    void func()
    {}

    class test
    {
    public:
        test()
        {
            (void)i;
            ++existing_instanses;
        }

        ~test()
        {
            --existing_instanses;
        }

        test(const test &) = delete;
        test &operator=(const test &) = delete;

        void operator()()
        {}

        void func()
        {}

        test(test &&) noexcept
        {
            ++existing_instanses;
            ++move_count;
        }

        test &operator=(test &&) noexcept
        {
            ++move_assign_count;
            return *this;
        }

        inline static size_t move_count = 0;
        inline static size_t move_assign_count = 0;
        
        inline static size_t existing_instanses = 0;

    private:
        int i = 0;
    };

    class big_test
    {
    public:
        big_test()
        {
            (void)b;
            ++existing_instanses;
        }

        ~big_test()
        {
            --existing_instanses;
        }

        big_test(const big_test &) = delete;
        big_test &operator=(const big_test &) = delete;

        void operator()()
        {}

        void func()
        {}

        big_test(big_test &&)
        {
            ++existing_instanses;
            ++move_count;
        }

        big_test &operator=(big_test &&)
        {
            ++move_assign_count;
            return *this;
        }

        inline static size_t move_count = 0;
        inline static size_t move_assign_count = 0;

        inline static size_t existing_instanses = 0;

    private:
        std::byte b[100];
    };

    class multiple_test
    {
    public:
        multiple_test()
        {
            (void)i;
        }

        int operator()(int)
        {
            return 0;
        }

        int operator()()
        {
            return 1;
        }

        int operator()(double)
        {
            return 2;
        }

    private:
        int i = 0;
    };

    class big_multiple_test
    {
    public:
        big_multiple_test()
        {
            (void)b;
        }

        int operator()(int)
        {
            return 0;
        }

        int operator()()
        {
            return 1;
        }

        int operator()(double)
        {
            return 2;
        }

    private:
        std::byte b[100];
    };

    class const_test
    {
    public:
        const_test()
        {
            (void)i;
        }

        int operator()() const
        {
            return 1;
        }

    private:
        int i = 0;
    };

    class big_const_test
    {
    public:
        big_const_test()
        {
            (void)b;
        }

        int operator()() const
        {
            return 1;
        }

    private:
        std::byte b[100];
    };
}

class Function
    : public Test
{
protected:
    void SetUp() override
    {
        test::move_count = 0;
        test::move_assign_count = 0;
        test::existing_instanses = 0;

        big_test::move_count = 0;
        big_test::move_assign_count = 0;
        big_test::existing_instanses = 0;
    }
};

TEST_F(Function, Init)
{
    function<void()> f1([]() {});
    const function<void()> f2([]() {});

    f1();
    f2();
}

TEST_F(Function, DefaultCreatable)
{
    function<void()> f;

    ASSERT_FALSE(f);
}

TEST_F(Function, MayContainAnyCallable)
{
    test t;
    big_test bt;

    function<void()> f1(func); f1();
    function<void()> f2(&func); f2();
    function<void()> f3(test{}); f3();
    function<void()> f4(big_test{}); f4();
    function<void(test &)> f5(&test::func); f5(t);
    function<void(big_test &)> f6(&big_test::func); f6(bt);
    function<void()> f7([]() {}); f7();
    function<void()> f8([a = std::array<char, 100>()]() { (void)a; }); f8();
    function<void()> f7_([]() mutable {}); f7_();
    function<void()> f8_([a = std::array<char, 100>()]() mutable { (void)a; }); f8_();
    function<void()> f9(std::function<void()>{ &func }); f9();

    const function<void()> f10(func); f10();
    const function<void()> f11(&func); f11();
    const function<void()> f12(test{}); f12();
    const function<void()> f13(big_test{}); f13();
    const function<void(test &)> f14(&test::func); f14(t);
    const function<void(big_test &)> f15(&big_test::func); f15(bt);
    const function<void()> f16([]() {}); f16();
    const function<void()> f17([a = std::array<char, 100>()]() { (void)a; }); f17();
    const function<void()> f16_([]() mutable {}); f16_();
    const function<void()> f17_([a = std::array<char, 100>()]() mutable { (void)a; }); f17_();
    const function<void()> f18(std::function<void()>{ &func }); f18();
}

TEST_F(Function, MayContainFunctorWithMultipleOperators)
{
    function<int()> f1(multiple_test{});
    EXPECT_EQ(f1(), 1);

    function<int(int)> f2(multiple_test{});
    EXPECT_EQ(f2(0), 0);

    function<int(double)> f3(multiple_test{});
    EXPECT_EQ(f3(0.0), 2);

    const function<int()> f4(multiple_test{});
    EXPECT_EQ(f4(), 1);

    const function<int(int)> f5(multiple_test{});
    EXPECT_EQ(f5(0), 0);

    const function<int(double)> f6(multiple_test{});
    EXPECT_EQ(f6(0.0), 2);

    function<int()> f7(big_multiple_test{});
    EXPECT_EQ(f7(), 1);

    function<int(int)> f8(big_multiple_test{});
    EXPECT_EQ(f8(0), 0);

    function<int(double)> f9(big_multiple_test{});
    EXPECT_EQ(f9(0.0), 2);

    const function<int()> f10(big_multiple_test{});
    EXPECT_EQ(f10(), 1);

    const function<int(int)> f11(big_multiple_test{});
    EXPECT_EQ(f11(0), 0);

    const function<int(double)> f12(big_multiple_test{});
    EXPECT_EQ(f12(0.0), 2);
}

TEST_F(Function, MayContainFunctorWithConstOperators)
{
    function<int()> f1(const_test{});
    EXPECT_EQ(f1(), 1);

    const function<int()> f2(const_test{});
    EXPECT_EQ(f2(), 1);

    function<int()> f3(big_const_test{});
    EXPECT_EQ(f3(), 1);

    const function<int()> f4(big_const_test{});
    EXPECT_EQ(f4(), 1);
}

TEST_F(Function, MoveConstructible)
{
    function<void()> f(test{});
    auto f1 = std::move(f); f1();

    EXPECT_FALSE(f);
    EXPECT_TRUE(f1);
    EXPECT_EQ(test::existing_instanses, 1);

    function<void()> f2(big_test{});
    auto f3 = std::move(f2); f3();

    EXPECT_FALSE(f2);
    EXPECT_TRUE(f3);
    EXPECT_EQ(big_test::existing_instanses, 1);

    function<void()> f4;
    auto f5 = std::move(f4);

    EXPECT_FALSE(f4);
    EXPECT_FALSE(f5);
}

TEST_F(Function, MoveConstructibleWithEmptyObject)
{
    function<void()> f;
    auto f1 = std::move(f);;

    EXPECT_FALSE(f);
    EXPECT_FALSE(f1);

    function<void()> f2;
    auto f3 = std::move(f2);

    EXPECT_FALSE(f2);
    EXPECT_FALSE(f3);
}

TEST_F(Function, DeletesObject)
{
    {
        function<void()> f(test{}); f();
    }
    EXPECT_EQ(test::existing_instanses, 0);
    
    {
        function<void()> f2(big_test{}); f2();
    }
    EXPECT_EQ(big_test::existing_instanses, 0);
}

TEST_F(Function, DeletesEmptyObject)
{
    {
        function<void()> f; (void)f;
    }
}

TEST_F(Function, MoveAssignable)
{
    {
        function<void()> f(test{});
        auto &f_alias = f;
        f = std::move(f_alias);
        EXPECT_EQ(test::existing_instanses, 1);
        EXPECT_TRUE(f);
    }
    EXPECT_EQ(test::existing_instanses, 0);

    {
        function<void()> f;
        auto &f_alias = f;
        f = std::move(f_alias);
        EXPECT_EQ(test::existing_instanses, 0);
        EXPECT_FALSE(f);
    }
    EXPECT_EQ(test::existing_instanses, 0);

    {
        function<void()> f(test{});
        function<void()> f1(test{});
        f1 = std::move(f); f1();
        EXPECT_TRUE(f1);
        EXPECT_FALSE(f);
        EXPECT_EQ(test::existing_instanses, 1);
    }
    EXPECT_EQ(test::existing_instanses, 0);

    {
        function<void()> f(test{});
        function<void()> f1;
        f1 = std::move(f); f1();
        EXPECT_TRUE(f1);
        EXPECT_FALSE(f);
        EXPECT_EQ(test::existing_instanses, 1);
    }
    EXPECT_EQ(test::existing_instanses, 0);

    {
        function<void()> f;
        function<void()> f1(test{});
        f1 = std::move(f);
        EXPECT_FALSE(f1);
        EXPECT_FALSE(f);
        EXPECT_EQ(test::existing_instanses, 0);
    }
    EXPECT_EQ(test::existing_instanses, 0);

    {
        function<void()> f;
        function<void()> f1;
        f1 = std::move(f);
        EXPECT_FALSE(f1);
        EXPECT_FALSE(f);
        EXPECT_EQ(test::existing_instanses, 0);
    }
    EXPECT_EQ(test::existing_instanses, 0);

    {
        function<void()> f(big_test{});
        function<void()> f1(big_test{});
        f1 = std::move(f); f1();
        EXPECT_TRUE(f1);
        EXPECT_FALSE(f);
        EXPECT_EQ(big_test::existing_instanses, 1);
    }
    EXPECT_EQ(big_test::existing_instanses, 0);

    {
        function<void()> f(big_test{});
        function<void()> f1;
        f1 = std::move(f); f1();
        EXPECT_TRUE(f1);
        EXPECT_FALSE(f);
        EXPECT_EQ(big_test::existing_instanses, 1);
    }
    EXPECT_EQ(big_test::existing_instanses, 0);

    {
        function<void()> f;
        function<void()> f1(big_test{});
        f1 = std::move(f);
        EXPECT_FALSE(f1);
        EXPECT_FALSE(f);
        EXPECT_EQ(big_test::existing_instanses, 0);
    }
    EXPECT_EQ(big_test::existing_instanses, 0);

    {
        function<void()> f;
        function<void()> f1;
        f1 = std::move(f);
        EXPECT_FALSE(f1);
        EXPECT_FALSE(f);
        EXPECT_EQ(big_test::existing_instanses, 0);
    }
    EXPECT_EQ(big_test::existing_instanses, 0);
}

TEST_F(Function, ThrowsExceptionOnAttemptToCallEmptyObject)
{
    function<void()> f;

    ASSERT_THROW(f(), std::bad_function_call);
}

TEST_F(Function, MoveParameter)
{
    std::unique_ptr<int> t;
    function<void(std::unique_ptr<int>)> f([&](std::unique_ptr<int> &&v) { t = std::move(v); });
    f(std::make_unique<int>(9));

    ASSERT_TRUE(t && *t == 9);
}

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif
TEST_F(Function, MayReturnTypesWithAnyQualifiers)
{
    static int t = 0;
    static volatile int vt = 0;

    function<int()> f1([]() -> int { return t; }); f1();
    function<int &()> f2([]() -> int &{ return t; }); f2();
    function<int &&()> f3([]() -> int &&{ return std::move(t); }); f3();
    function<const int()> f4([]() -> const int { return t; }); f4();
    function<const int &()> f5([]() -> const int &{ return t; }); f5();
    function<const int &&()> f6([]() -> const int &&{ return std::move(t); }); f6();
    function<volatile int()> f7([]() -> volatile int { return t; }); f7();
    function<volatile int &()> f8([]() -> volatile int &{ return vt; });
    volatile int &r8 = f8(); EXPECT_EQ(&r8, &vt);
    function<volatile int &&()> f9([]() -> volatile int &&{ return std::move(vt); });
    volatile int &&r9 = f9(); EXPECT_EQ(&r9, &vt);
    function<const volatile int()> f10([]() -> const volatile int { return vt; });
    const volatile int r10 = f10(); EXPECT_NE(&r10, &vt);
    function<const volatile int &()> f11([]() -> const volatile int &{ return vt; });
    const volatile int &r11 = f11(); EXPECT_EQ(&r11, &vt);
    function<const volatile int &&()> f12([]() -> const volatile int &&{ return std::move(vt); });
    const volatile int &&r12 = f12(); EXPECT_EQ(&r12, &vt);
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

TEST_F(Function, MayHaveTypeWithAnyQualifiersAsArgumtn)
{
    static int t = 0;

    function<void(int)> f1([](int) { }); f1(t);
    function<void(int &)> f2([](int &) { }); f2(t);
    function<void(int &&)> f3([](int &&) { }); f3(std::move(t));
    function<void(const int)> f4([](const int) { }); f4(t);
    function<void(const int &)> f5([](const int &) { }); f5(t);
    function<void(const int &&)> f6([](const int &&) { }); f6(std::move(t));
    function<void(volatile int)> f7([](volatile int) { }); f7(t);
    function<void(volatile int &)> f8([](volatile int &) { }); f8(t);
    function<void(volatile int &&)> f9([](volatile int &&) { }); f9(std::move(t));
    function<void(const volatile int)> f10([](const volatile int) { }); f10(t);
    function<void(const volatile int &)> f11([](const volatile int &) { }); f11(t);
    function<void(const volatile int &&)> f12([](const volatile int &&) { }); f12(std::move(t));
}
