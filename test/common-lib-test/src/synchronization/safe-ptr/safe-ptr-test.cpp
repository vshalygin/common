#include <common-lib/synchronization/safe-ptr.h>

#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

namespace {
    struct test
    {
        test()
        {
            ++existing_instances;
        }

        test(test &&) = delete;
        test &operator=(test &&) = delete;

        ~test()
        {
            --existing_instances;
        }

        int get_val() const
        {
            return val;
        }

        void set_val(int v)
        {
            val = v;
        }

        int val = 0;

        inline static int existing_instances = 0;
    };
}

TEST(SafePtr, Init)
{
    auto sut = make_safe<test>();
    EXPECT_EQ(sut->val, 0);
    EXPECT_EQ(sut->get_val(), 0);

    sut->set_val(4);
    EXPECT_EQ(sut->val, 4);
    EXPECT_EQ(sut->get_val(), 4);

    sut->val = 8;
    EXPECT_EQ(sut->val, 8);
    EXPECT_EQ(sut->get_val(), 8);
}

TEST(SafePtr, StoringObjectMayBeChangedForConstSafePtr)
{
    const auto sut = make_safe<test>();
    EXPECT_EQ(sut->val, 0);
    EXPECT_EQ(sut->get_val(), 0);

    sut->set_val(4);
    EXPECT_EQ(sut->val, 4);
    EXPECT_EQ(sut->get_val(), 4);

    sut->val = 8;
    EXPECT_EQ(sut->val, 8);
    EXPECT_EQ(sut->get_val(), 8);
}

TEST(SafePtr, IsLockable)
{
    auto sut = make_safe<test>();

    std::lock_guard g(sut);
    EXPECT_EQ(sut->val, 0);
    EXPECT_EQ(sut->get_val(), 0);

    sut->set_val(4);
    EXPECT_EQ(sut->val, 4);
    EXPECT_EQ(sut->get_val(), 4);

    sut->val = 8;
    EXPECT_EQ(sut->val, 8);
    EXPECT_EQ(sut->get_val(), 8);
}

TEST(SafePtr, IsConvertibleToBool)
{
    safe_ptr<test> sut;
    ASSERT_FALSE(static_cast<bool>(sut));

    sut = make_safe<test>();
    ASSERT_TRUE(static_cast<bool>(sut));
}

TEST(SafePtr, DeletesStoringObjectsOnDestruction)
{
    {
        auto sut = make_safe<test>();
        sut;
    }

    EXPECT_EQ(test::existing_instances, 0);
}

TEST(SafePtr, ResetsWithAnotherStoringObject)
{
    auto sut = make_safe<test>();
    sut->val = 34;

    sut.reset(new test);

    EXPECT_EQ(sut->val, 0);
    EXPECT_EQ(test::existing_instances, 1);
}

TEST(SafePtr, Swap)
{
    auto sut1 = make_safe<test>();
    auto sut2 = make_safe<test>();
    sut1->val = 34;
    sut2->val = 60;

    sut1.swap(sut2);

    EXPECT_EQ(sut1->val, 60);
    EXPECT_EQ(sut2->val, 34);
}

TEST(SafePtr, UseCount)
{
    auto sut = make_safe<test>();
    auto sut2 = sut;
    auto sut3 = sut2;

    EXPECT_EQ(sut3.use_count(), 3);
}

TEST(SafePtr, HasSharedPtrSemantics)
{
    auto sut = make_safe<test>();
    sut->val = 34;
    auto sut1 = sut;

    sut.reset();

    ASSERT_EQ(sut1->val, 34);
}
