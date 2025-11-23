#include <common-lib/memory/unique-ptr/unique-ptr.h>

#include <gtest/gtest.h>

using namespace vsh::common_lib;
using namespace testing;

namespace {
    //TODO move to common test library
    class life_cycle_tracker
    {
    public:
        life_cycle_tracker()
        {
            ++ctor_called;
        }

        ~life_cycle_tracker()
        {
            ++dtor_called;
        }

        life_cycle_tracker(const life_cycle_tracker &)
        {
            ++copy_called;
        }

        life_cycle_tracker &operator=(life_cycle_tracker &)
        {
            ++copy_called;
        }

        life_cycle_tracker(life_cycle_tracker &&)
        {
            ++copy_assign_called;
        }

        life_cycle_tracker &operator=(life_cycle_tracker &&)
        {
            ++move_assign_called;
        }

        inline static int ctor_called = 0;
        inline static int dtor_called = 0;
        inline static int copy_called = 0;
        inline static int move_called = 0;
        inline static int copy_assign_called = 0;
        inline static int move_assign_called = 0;

        inline static void drop_counters()
        {
            ctor_called = 0;
            dtor_called = 0;
            copy_called = 0;
            move_called = 0;
            copy_assign_called = 0;
            move_assign_called = 0;
        }
    };

    class test_class
        : private life_cycle_tracker
    {
    public:
        void call_method() const
        {
            ++method_called;
        }

        inline static int method_called = 0;
    };
}

class UniquePtr
    : public Test
{
protected:
    void SetUp() override
    {
        life_cycle_tracker::drop_counters();
        test_class::method_called = 0;
    }
};

TEST_F(UniquePtr, MayBeCreated)
{
    make_unique<test_class>();

    EXPECT_EQ(life_cycle_tracker::ctor_called, 1);
    EXPECT_EQ(life_cycle_tracker::dtor_called, 1);
    EXPECT_EQ(life_cycle_tracker::copy_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_called, 0);
    EXPECT_EQ(life_cycle_tracker::copy_assign_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_assign_called, 0);
}

TEST_F(UniquePtr, MayBeDereferenced)
{
    auto sut = make_unique<test_class>();
    (*sut).call_method();

    ASSERT_TRUE(test_class::method_called == 1);
}

TEST_F(UniquePtr, ConstMayBeDereferenced)
{
    const auto sut = make_unique<test_class>();
    (*sut).call_method();

    ASSERT_TRUE(test_class::method_called == 1);
}

TEST_F(UniquePtr, MayBeIndirectAccessed)
{
    auto sut = make_unique<test_class>();
    sut->call_method();

    ASSERT_TRUE(test_class::method_called == 1);
}

TEST_F(UniquePtr, ConstMayBeIndirectAccessed)
{
    const auto sut = make_unique<test_class>();
    sut->call_method();

    ASSERT_TRUE(test_class::method_called == 1);
}

TEST_F(UniquePtr, ConvertsToFalseIfEmpty)
{
    unique_ptr<test_class> sut;

    ASSERT_FALSE(sut);
}

TEST_F(UniquePtr, ConvertsToTrueIfNotEmpty)
{
    auto sut = make_unique<test_class>();

    ASSERT_TRUE(sut);
}

TEST_F(UniquePtr, MayBeMoved)
{
    {
        auto ptr1 = make_unique<test_class>();

        unique_ptr<test_class> ptr2(std::move(ptr1));
        ptr2->call_method();
    }

    EXPECT_EQ(life_cycle_tracker::ctor_called, 1);
    EXPECT_EQ(life_cycle_tracker::dtor_called, 1);
    EXPECT_EQ(life_cycle_tracker::copy_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_called, 0);
    EXPECT_EQ(life_cycle_tracker::copy_assign_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_assign_called, 0);
    EXPECT_EQ(test_class::method_called, 1);
}

TEST_F(UniquePtr, MayBeMoveAssigned)
{
    {
        auto ptr1 = make_unique<test_class>();
        auto ptr1_inner = ptr1.get();

        auto ptr2 = make_unique<test_class>();
        ptr2 = std::move(ptr1);

        EXPECT_EQ(ptr2.get(), ptr1_inner);
    }

    EXPECT_EQ(life_cycle_tracker::ctor_called, 2);
    EXPECT_EQ(life_cycle_tracker::dtor_called, 2);
    EXPECT_EQ(life_cycle_tracker::copy_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_called, 0);
    EXPECT_EQ(life_cycle_tracker::copy_assign_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_assign_called, 0);
}

TEST_F(UniquePtr, ReturnsNullptrOnGetOperationIsObjectIsEmpty)
{
    unique_ptr<test_class> sut;

    ASSERT_EQ(sut.get(), nullptr);
}

TEST_F(UniquePtr, ReturnsValidPointerOnGetOperation)
{
    auto sut = make_unique<test_class>();
    
    sut.get()->call_method();

    ASSERT_EQ(test_class::method_called, 1);
}

TEST_F(UniquePtr, DestroysObjectAfterResetOperation)
{
    auto ptr = make_unique<test_class>();

    ptr.reset();

    ASSERT_FALSE(ptr);
    EXPECT_EQ(life_cycle_tracker::ctor_called, 1);
    EXPECT_EQ(life_cycle_tracker::dtor_called, 1);
    EXPECT_EQ(life_cycle_tracker::copy_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_called, 0);
    EXPECT_EQ(life_cycle_tracker::copy_assign_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_assign_called, 0);
}
