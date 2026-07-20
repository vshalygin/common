#include <common-lib/memory/enable-shared-from-this-manual-set.h>

#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

namespace {
    class test
        : public enable_shared_from_this_manual_set<test>
    {
    public:
        void set(int v)
        {
            m_val = v;
        }

        int get() const
        {
            return m_val;
        }

        auto get_shared_ptr()
        {
            return shared_from_this();
        }

        auto get_shared_ptr_const() const
        {
            return shared_from_this();
        }

        auto get_weak_ptr()
        {
            return weak_from_this();
        }

        auto get_weak_ptr_const() const
        {
            return weak_from_this();
        }

    private:
        int m_val = 0;
    };

    class derived_test
        : public test
    {};
}

TEST(EnableSharedFromThisManualSet, Init)
{
    auto sut = std::make_shared<test>();
    sut->set_self_shared_ptr(sut);

    sut->set(34);

    ASSERT_EQ(sut->get_shared_ptr_const()->get(), 34);
}

TEST(EnableSharedFromThisManualSet, TestNotInitialized)
{
    auto sut = std::make_shared<test>();

    EXPECT_THROW(sut->get_shared_ptr(), std::bad_weak_ptr);
    EXPECT_THROW(sut->get_shared_ptr_const(), std::bad_weak_ptr);
    EXPECT_TRUE(sut->get_weak_ptr().expired());
    EXPECT_TRUE(sut->get_weak_ptr_const().expired());
}

TEST(EnableSharedFromThisManualSet, TestSharedFromThis)
{
    auto sut = std::make_shared<test>();
    sut->set_self_shared_ptr(sut);

    auto sp1 = sut->get_shared_ptr();
    auto sp2 = sut->get_shared_ptr_const();

    sp1->set(45);

    EXPECT_EQ(sp1->get(), 45);
    EXPECT_EQ(sp2->get(), 45);
}

TEST(EnableSharedFromThisManualSet, TestWeakFromThis)
{
    auto sut = std::make_shared<test>();
    sut->set_self_shared_ptr(sut);

    auto sp1 = sut->get_weak_ptr();
    auto sp2 = sut->get_weak_ptr_const();

    sp1.lock()->set(45);

    EXPECT_EQ(sp1.lock()->get(), 45);
    EXPECT_EQ(sp1.lock()->get(), 45);
}
