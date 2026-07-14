#include <common-lib/synchronization/value-locker.h>

#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

namespace {
    struct test
    {
        test(int v)
            : val(v)
        {}

        int get_val() const
        {
            return val;
        }

        void set_val(int v)
        {
            val = v;
        }

        int val;
    };
}

TEST(ValueLocker, Init)
{
    value_locker<test> t(7);
    EXPECT_EQ(t.lock()->get_val(), 7);
    EXPECT_EQ(t.lock()->val, 7);
    EXPECT_EQ((*t.lock()).get_val(), 7);
    EXPECT_EQ((*t.lock()).val, 7);

    t.lock()->set_val(8);
    EXPECT_EQ(t.lock()->get_val(), 8);
    EXPECT_EQ(t.lock()->val, 8);
    EXPECT_EQ((*t.lock()).get_val(), 8);
    EXPECT_EQ((*t.lock()).val, 8);

    t.lock()->val = 9;
    EXPECT_EQ(t.lock()->get_val(), 9);
    EXPECT_EQ(t.lock()->val, 9);
    EXPECT_EQ((*t.lock()).get_val(), 9);
    EXPECT_EQ((*t.lock()).val, 9);

    (*t.lock()).set_val(1);
    EXPECT_EQ(t.lock()->get_val(), 1);
    EXPECT_EQ(t.lock()->val, 1);
    EXPECT_EQ((*t.lock()).get_val(), 1);
    EXPECT_EQ((*t.lock()).val, 1);

    (*t.lock()).val = 2;
    EXPECT_EQ(t.lock()->get_val(), 2);
    EXPECT_EQ(t.lock()->val, 2);
    EXPECT_EQ((*t.lock()).get_val(), 2);
    EXPECT_EQ((*t.lock()).val, 2);
}

TEST(ValueLocker, ConstObject)
{
    const value_locker<test> t(7);

    EXPECT_EQ(t.lock()->get_val(), 7);
    EXPECT_EQ(t.lock()->val, 7);
    EXPECT_EQ((*t.lock()).get_val(), 7);
    EXPECT_EQ((*t.lock()).val, 7);
}

TEST(ValueLocker, StoreLockedObject)
{
    value_locker<test> t(7);
    auto locker = t.lock();
    EXPECT_EQ(locker->get_val(), 7);
    EXPECT_EQ(locker->val, 7);
    EXPECT_EQ((*locker).get_val(), 7);
    EXPECT_EQ((*locker).val, 7);

    locker->set_val(8);
    EXPECT_EQ(locker->get_val(), 8);
    EXPECT_EQ(locker->val, 8);
    EXPECT_EQ((*locker).get_val(), 8);
    EXPECT_EQ((*locker).val, 8);
}

TEST(ValueLocker, ConstStoreLockedObject)
{
    value_locker<test> t(7);
    const auto locker = t.lock();
    EXPECT_EQ(locker->get_val(), 7);
    EXPECT_EQ(locker->val, 7);
    EXPECT_EQ((*locker).get_val(), 7);
    EXPECT_EQ((*locker).val, 7);
}

TEST(ValueLocker, ConstStoreLockedConstObject)
{
    const value_locker<test> t(7);
    auto locker = t.lock();

    EXPECT_EQ(locker->get_val(), 7);
    EXPECT_EQ(locker->val, 7);
    EXPECT_EQ((*locker).get_val(), 7);
    EXPECT_EQ((*locker).val, 7);
}
