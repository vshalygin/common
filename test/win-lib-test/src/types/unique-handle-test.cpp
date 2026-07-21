#include <win-lib/types/internal/unique-handle.h>

#include <gtest/gtest.h>
#include <set>

using namespace vshalygin::win;
using namespace vshalygin::win::internal;
using namespace testing;

namespace {
    struct test_handle_traits
    {
        using handle_type = int;

        inline static std::set<int> opened;
        inline static int next = 1;

        static int open() {
            auto n = ++next;
            opened.insert(n);
            return n;
        }

        static int invalid() noexcept
        {
            return 0;
        }

        static void close(int h) noexcept
        {
            assert(opened.count(h));
            opened.erase(h);
        }
    };

    using test_handle = unique_handle<test_handle_traits>;
}

class UniqueHandle
    : public Test
{
protected:
    void SetUp() override
    {
        test_handle_traits::opened.clear();
        test_handle_traits::next = 1;
    }

    void TearDown() override
    {
        ASSERT_TRUE(test_handle_traits::opened.empty());
    }
};

TEST_F(UniqueHandle, TestEmptyHandle)
{
    test_handle handle;

    ASSERT_FALSE(handle);
    ASSERT_TRUE(handle.empty());
    ASSERT_EQ(handle.get(), test_handle_traits::invalid());
    ASSERT_EQ(*handle.put(), test_handle_traits::invalid());
    ASSERT_EQ(*handle.addressof(), test_handle_traits::invalid());
    handle.reset();
    ASSERT_EQ(handle.release(), test_handle_traits::invalid());
}

TEST_F(UniqueHandle, TestHandleWithStoringValue)
{
    test_handle handle(test_handle_traits::open());

    ASSERT_TRUE(handle);
    ASSERT_FALSE(handle.empty());
    ASSERT_NE(handle.get(), test_handle_traits::invalid());
    ASSERT_NE(*handle.addressof(), test_handle_traits::invalid());
}

TEST_F(UniqueHandle, MoveCopyable)
{
    test_handle handle1(test_handle_traits::open());
    test_handle handle2(std::move(handle1));
    EXPECT_FALSE(handle1);
    EXPECT_TRUE(handle2);

    test_handle handle3;
    test_handle handle4(std::move(handle3));
    EXPECT_FALSE(handle3);
    EXPECT_FALSE(handle4);
}

TEST_F(UniqueHandle, MoveAssignable)
{
    test_handle handle1;
    test_handle handle2;
    handle1 = std::move(handle1);
    handle2 = std::move(handle1);
    EXPECT_FALSE(handle1);
    EXPECT_FALSE(handle2);

    test_handle handle3(test_handle_traits::open());
    test_handle handle4;
    handle3 = std::move(handle3);
    handle4 = std::move(handle3);
    EXPECT_FALSE(handle3);
    EXPECT_TRUE(handle4);

    test_handle handle5;
    test_handle handle6(test_handle_traits::open());
    handle5 = std::move(handle5);
    handle6 = std::move(handle5);
    EXPECT_FALSE(handle5);
    EXPECT_FALSE(handle6);

    test_handle handle7(test_handle_traits::open());
    test_handle handle8(test_handle_traits::open());
    handle7 = std::move(handle7);
    handle8 = std::move(handle7);
    EXPECT_FALSE(handle7);
    EXPECT_TRUE(handle8);
}

TEST_F(UniqueHandle, TestReset)
{
    test_handle handle1;
    handle1.reset();
    EXPECT_FALSE(handle1);

    test_handle handle2;
    handle2.reset(test_handle_traits::open());
    EXPECT_TRUE(handle2);

    test_handle handle3(test_handle_traits::open());
    handle3.reset();
    EXPECT_FALSE(handle3);

    auto h = test_handle_traits::open();
    test_handle handle4(h);
    handle4.reset(h);
    EXPECT_EQ(handle4.get(), h);
    EXPECT_TRUE(handle4);

    test_handle handle5(test_handle_traits::open());
    handle5.reset(test_handle_traits::open());
    EXPECT_TRUE(handle5);
}

TEST_F(UniqueHandle, TestGet)
{
    test_handle handle1;
    EXPECT_EQ(handle1.get(), test_handle_traits::invalid());

    auto h = test_handle_traits::open();
    test_handle handle2(h);
    EXPECT_EQ(handle2.get(), h);
    EXPECT_TRUE(handle2);
}

TEST_F(UniqueHandle, TestRelease)
{
    test_handle handle1;
    EXPECT_EQ(handle1.release(), test_handle_traits::invalid());

    auto h = test_handle_traits::open();
    test_handle handle2(h);
    auto hh = handle2.release();
    EXPECT_EQ(hh, h);
    EXPECT_FALSE(handle2);

    test_handle_traits::close(hh);
}

TEST_F(UniqueHandle, TestPut)
{
    test_handle handle1;
    EXPECT_EQ(*handle1.put(), test_handle_traits::invalid());

    auto h = test_handle_traits::open();
    test_handle handle2(h);
    EXPECT_EQ(*handle2.put(), test_handle_traits::invalid());
    EXPECT_FALSE(handle2);
}

TEST_F(UniqueHandle, TestAddressOf)
{
    test_handle handle1;
    EXPECT_EQ(*handle1.addressof(), test_handle_traits::invalid());

    auto h = test_handle_traits::open();
    test_handle handle2(h);
    EXPECT_EQ(*handle2.addressof(), h);
    EXPECT_TRUE(handle2);
}

TEST_F(UniqueHandle, TestSwapMethod)
{
    auto h1 = test_handle_traits::open();
    auto h2 = test_handle_traits::open();
    test_handle handle1(h1);
    test_handle handle2(h2);

    handle1.swap(handle2);

    EXPECT_EQ(handle1.get(), h2);
    EXPECT_EQ(handle2.get(), h1);
}

TEST_F(UniqueHandle, TestSwapFreeFunction)
{
    auto h1 = test_handle_traits::open();
    auto h2 = test_handle_traits::open();
    test_handle handle1(h1);
    test_handle handle2(h2);

    swap(handle1, handle2);

    EXPECT_EQ(handle1.get(), h2);
    EXPECT_EQ(handle2.get(), h1);
}
