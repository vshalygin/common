#include <common-lib/memory/unique-ptr2/unique-ptr2.h>

#include <gtest/gtest.h>

#include <set>

using namespace vsh::cl;
using namespace testing;

namespace {
    //TODO move to common test library
    class test_allocator
    {
    public:
        template<typename T>
        T *allocate() const
        {
            once_allocated = true;
            auto ans = allocator.allocate<T>();
            allocated_memory.insert(static_cast<void *>(ans));
            return ans;
        }

        void deallocate(void *ptr) const noexcept
        {
            auto it = allocated_memory.find(ptr);
            ASSERT_TRUE(it != allocated_memory.end());
            allocated_memory.erase(it);

            allocator.deallocate(ptr);
        }

        inline static std::set<void *> allocated_memory;
        inline static bool once_allocated = false;

        inline static void clear()
        {
            allocated_memory.clear();
            once_allocated = true;
        }

    private:
        default_allocator allocator;
    };

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

        life_cycle_tracker &operator=(const life_cycle_tracker &)
        {
            ++copy_assign_called;
        }

        life_cycle_tracker(life_cycle_tracker &&)
        {
            ++move_called;
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

    class virual_base
    {
    public:
        virtual ~virual_base() = default;

        void set_virual_base(int val)
        {
            val_ = val;
        }

        int get_virual_base()
        {
            return val_;
        }

    private:
        char buf_[121];
        int val_ = 0;
    };

    class test_base
        : public virtual life_cycle_tracker
    {
    public:
        virtual ~test_base() = default;

        test_base() = default;

        test_base(const test_base &) = default;
        test_base &operator=(const test_base &) = default;

        test_base(test_base &&) = default;
        test_base &operator=(test_base &&) = default;

        void set_test_base(int val) const
        {
            val_ = val;
        }

        int get_test_base()  const
        {
            return val_;
        }

    private:
        char buf_[99];
        mutable int val_ = 0;
    };

    class test_base_1
        : public test_base
    {
    public:
        virtual ~test_base_1() = default;

        void set_test_base1(int val) const
        {
            val_ = val;
        }

        int get_test_base1() const
        {
            return val_;
        }

    private:
        mutable int val_ = 0;
    };

    class test_base_2
        : public test_base
    {
    public:
        virtual ~test_base_2() = default;

        double get_test_base2() const
        {
            return val_;
        }

        void set_test_base2(double val) const
        {
            val_ = val;
        }

    private:
        char block[100];
        mutable double val_ = 0;
    };

    class test_base_3_with_virtual_base
        : virtual public virual_base
    {
    public:
        virtual ~test_base_3_with_virtual_base() = default;

        double get_test_base3() const
        {
            return val_;
        }

        void set_test_base3(double val) const
        {
            val_ = val;
        }

    private:
        char block[103];
        mutable double val_ = 0;
    };

    class test_base_4_with_virtual_base
        : virtual public virual_base
    {
    public:
        virtual ~test_base_4_with_virtual_base() = default;

        double get_test_base_4_with_virtual_base() const
        {
            return val_;
        }

        void set_test_base_4_with_virtual_base(double val) const
        {
            val_ = val;
        }

    private:
        char block[104];
        mutable double val_ = 0;
    };

    class test_class_with_one_base
        : public test_base_1
    {
    public:
        virtual ~test_class_with_one_base() = default;

        int val_ = 0;
    };

    class test_class_with_two_base
        : public test_base_1
        , public test_base_2
    {
    public:
        virtual ~test_class_with_two_base() = default;

    private:
        char buf[5];
    };

    class test_class_with_two_base_with_virtual_base
        : public test_base_3_with_virtual_base
        , public test_base_4_with_virtual_base
    {
    public:
        virtual ~test_class_with_two_base_with_virtual_base() = default;

    private:
        char buf[5];
    };

    class super_test_class
        : public test_class_with_two_base
        , public test_class_with_two_base_with_virtual_base
    {
    public:
        ~super_test_class() override
        {
            ++destroyed_times;
        }

        double get_super_test_class() const
        {
            return val_;
        }

        void set_super_test_class(double val) const
        {
            val_ = val;
        }

        char buff_[78];
        mutable double val_ = 0;
        inline static int destroyed_times = 0;
    };

    class test_class_with_parameteraized_ctor
    {
    public:
        explicit test_class_with_parameteraized_ctor(test_base &&m)
            : member_(std::move(m))
        {}

        explicit test_class_with_parameteraized_ctor(const test_base &m)
            : member_(m)
        {}

    private:
        test_base member_;
    };

    class throw_ctor_class
    {
    public:
        throw_ctor_class()
        {
            throw std::exception();
        }
    };

    template<typename T>
    using test_unique_ptr2 = unique_ptr2<T, test_allocator>;

    template<typename T, typename...Args>
    test_unique_ptr2<T> make_test_unique2(Args&&...args)
    {
        return make_unique2<T, test_allocator>(test_allocator(), std::forward<Args>(args)...);
    }
}

class UniquePtr2
    : public Test
{
protected:
    void SetUp() override
    {
        life_cycle_tracker::drop_counters();
        test_allocator::clear();
        super_test_class::destroyed_times = 0;
    }

    void TearDown() override
    {
        ASSERT_TRUE(test_allocator::allocated_memory.empty());
        ASSERT_TRUE(test_allocator::once_allocated);
    }
};

TEST_F(UniquePtr2, MayBeCreated)
{
    make_test_unique2<test_base>();

    EXPECT_EQ(life_cycle_tracker::ctor_called, 1);
    EXPECT_EQ(life_cycle_tracker::dtor_called, 1);
    EXPECT_EQ(life_cycle_tracker::copy_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_called, 0);
    EXPECT_EQ(life_cycle_tracker::copy_assign_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_assign_called, 0);
}

TEST_F(UniquePtr2, MayBeDereferenced)
{
    auto sut = make_test_unique2<test_base>();
    (*sut).set_test_base(34);

    int expected = (*sut).get_test_base();
    ASSERT_TRUE(expected == 34);
}

TEST_F(UniquePtr2, ConstMayBeDereferenced)
{
    const auto sut = make_test_unique2<test_base>();
    (*sut).set_test_base(34);

    int expected = (*sut).get_test_base();
    ASSERT_TRUE(expected == 34);
}

TEST_F(UniquePtr2, MayBeIndirectAccessed)
{
    auto sut = make_test_unique2<test_base>();
    sut->set_test_base(34);

    int expected = (*sut).get_test_base();
    ASSERT_TRUE(expected == 34);
}

TEST_F(UniquePtr2, ConstMayBeIndirectAccessed)
{
    const auto sut = make_test_unique2<test_base>();
    sut->set_test_base(34);

    int expected = (*sut).get_test_base();
    ASSERT_TRUE(expected == 34);
}

TEST_F(UniquePtr2, ConvertsToFalseIfEmpty)
{
    test_unique_ptr2<test_base> sut;

    ASSERT_FALSE(sut);
}

TEST_F(UniquePtr2, ConvertsToTrueIfNotEmpty)
{
    auto sut = make_test_unique2<test_base>();

    ASSERT_TRUE(sut);
}

TEST_F(UniquePtr2, MayBeMoved)
{
    {
        auto ptr1 = make_test_unique2<test_base>();

        test_unique_ptr2<test_base> ptr2(std::move(ptr1));
        ptr2->set_test_base(34);
    }

    EXPECT_EQ(life_cycle_tracker::ctor_called, 1);
    EXPECT_EQ(life_cycle_tracker::dtor_called, 1);
    EXPECT_EQ(life_cycle_tracker::copy_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_called, 0);
    EXPECT_EQ(life_cycle_tracker::copy_assign_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_assign_called, 0);
}

TEST_F(UniquePtr2, MayBeMoveAssigned)
{
    {
        auto ptr1 = make_test_unique2<test_base>();
        auto ptr1_inner = ptr1.get();

        auto ptr2 = make_test_unique2<test_base>();
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

TEST_F(UniquePtr2, ReturnsNullptrOnGetOperationIsObjectIsEmpty)
{
    test_unique_ptr2<test_base> sut;

    ASSERT_EQ(sut.get(), nullptr);
}

TEST_F(UniquePtr2, ReturnsValidPointerOnGetOperation)
{
    auto sut = make_test_unique2<test_base>();
    
    sut.get()->set_test_base(34);

    int expected = (*sut).get_test_base();
    ASSERT_TRUE(expected == 34);
}

TEST_F(UniquePtr2, DestroysObjectAfterResetOperation)
{
    auto ptr = make_test_unique2<test_base>();

    ptr.reset();

    ASSERT_FALSE(ptr);
    EXPECT_EQ(life_cycle_tracker::ctor_called, 1);
    EXPECT_EQ(life_cycle_tracker::dtor_called, 1);
    EXPECT_EQ(life_cycle_tracker::copy_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_called, 0);
    EXPECT_EQ(life_cycle_tracker::copy_assign_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_assign_called, 0);
}

TEST_F(UniquePtr2, DoesNothingIfResetOperationCalledForEmptyObject)
{
    test_unique_ptr2<test_base> ptr;

    ptr.reset();

    EXPECT_EQ(life_cycle_tracker::ctor_called, 0);
    EXPECT_EQ(life_cycle_tracker::dtor_called, 0);
    EXPECT_EQ(life_cycle_tracker::copy_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_called, 0);
    EXPECT_EQ(life_cycle_tracker::copy_assign_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_assign_called, 0);
}

TEST_F(UniquePtr2, MayBeConvertedToBaseTypeIfObjectWithOneBase)
{
    auto ptr = make_test_unique2<test_class_with_one_base>();
    test_unique_ptr2<test_base_1> ptr1(std::move(ptr));

    ptr1->set_test_base1(34);

    EXPECT_EQ(ptr1->get_test_base1(), 34);
}

TEST_F(UniquePtr2, DoesNotCreateAnotherObjectAfterConversationFromOneTypeToAnother)
{
    auto ptr = make_test_unique2<test_class_with_one_base>();
    test_unique_ptr2<test_base_1> ptr1(std::move(ptr));

    ptr.reset();
    ptr1.reset();

    EXPECT_EQ(life_cycle_tracker::ctor_called, 1);
    EXPECT_EQ(life_cycle_tracker::dtor_called, 1);
    EXPECT_EQ(life_cycle_tracker::copy_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_called, 0);
    EXPECT_EQ(life_cycle_tracker::copy_assign_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_assign_called, 0);
}



TEST_F(UniquePtr2, MayBeConvertedToTheFirstBaseType)
{
    auto ptr = make_test_unique2<test_class_with_two_base>();
    test_unique_ptr2<test_base_1> ptr1(std::move(ptr));

    ptr1->set_test_base1(35);

    EXPECT_EQ(ptr1->get_test_base1(), 35);
}

TEST_F(UniquePtr2, IsEmptyAfterMoving)
{
    auto ptr = make_test_unique2<test_class_with_two_base>();
    test_unique_ptr2<test_base_1>(std::move(ptr));

    EXPECT_FALSE(ptr);
}

TEST_F(UniquePtr2, MayBeConvertedToTheSecondBaseType)
{
    auto ptr = make_test_unique2<test_class_with_two_base>();
    test_unique_ptr2<test_base_2> ptr1(std::move(ptr));

    ptr1->set_test_base2(36);

    EXPECT_EQ(ptr1->get_test_base2(), 36);
}

TEST_F(UniquePtr2, MayBeConvertedTwiceToBaseTypes)
{
    auto ptr = make_test_unique2<test_class_with_two_base>();
    test_unique_ptr2<test_base_2> ptr1(std::move(ptr));
    test_unique_ptr2<test_base> ptr2(std::move(ptr1));

    ptr2->set_test_base(36);

    EXPECT_EQ(ptr2->get_test_base(), 36);
}

TEST_F(UniquePtr2, MayBeConvertedToBaseTypeWithInIndirectPath)
{
    auto ptr = make_test_unique2<super_test_class>();
    test_unique_ptr2<test_class_with_two_base_with_virtual_base> ptr1(std::move(ptr));
    test_unique_ptr2<test_base_4_with_virtual_base> ptr2(std::move(ptr1));

    ptr2->set_test_base_4_with_virtual_base(36);

    EXPECT_EQ(ptr2->get_test_base_4_with_virtual_base(), 36);
}

TEST_F(UniquePtr2, MayBeConvertedToVirtualBase)
{
    auto ptr = make_test_unique2<super_test_class>();
    test_unique_ptr2<virual_base> ptr1(std::move(ptr));

    ptr1->set_virual_base(34);

    EXPECT_EQ(ptr1->get_virual_base(), 34);
}

TEST_F(UniquePtr2, MayBeMoveAssignedToBaseTypeIfObjectWithOneBase)
{
    test_unique_ptr2<test_base_1> ptr;
    ptr = make_test_unique2<test_class_with_one_base>();

    ptr->set_test_base1(34);

    EXPECT_EQ(ptr->get_test_base1(), 34);
}

TEST_F(UniquePtr2, DoesNotCreateAnotherObjectAfterMoveAssignedFromOneTypeToAnother)
{
    test_unique_ptr2<test_base_1> ptr;
    ptr = make_test_unique2<test_class_with_one_base>();

    ptr.reset();
    ptr.reset();

    EXPECT_EQ(life_cycle_tracker::ctor_called, 1);
    EXPECT_EQ(life_cycle_tracker::dtor_called, 1);
    EXPECT_EQ(life_cycle_tracker::copy_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_called, 0);
    EXPECT_EQ(life_cycle_tracker::copy_assign_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_assign_called, 0);
}

TEST_F(UniquePtr2, MayBeMoveAssignedToTheFirstBaseType)
{
    test_unique_ptr2<test_base_1> ptr1;
    ptr1 = make_test_unique2<test_class_with_two_base>();

    ptr1->set_test_base1(35);

    EXPECT_EQ(ptr1->get_test_base1(), 35);
}

TEST_F(UniquePtr2, IsEmptyAfterMoveAssigned)
{
    auto ptr = make_test_unique2<test_class_with_two_base>();
    test_unique_ptr2<test_base_1> ptr1;
    ptr1 = std::move(ptr);

    EXPECT_FALSE(ptr);
}

TEST_F(UniquePtr2, MayBeMoveAssignedToTheSecondBaseType)
{
    test_unique_ptr2<test_base_2> ptr1;
    ptr1 = make_test_unique2<test_class_with_two_base>();

    ptr1->set_test_base2(36);

    EXPECT_EQ(ptr1->get_test_base2(), 36);
}

TEST_F(UniquePtr2, MayBeMoveAssignedTwiceToBaseTypes)
{
    test_unique_ptr2<test_base_2> ptr1;
    ptr1 = make_test_unique2<test_class_with_two_base>();
    test_unique_ptr2<test_base> ptr2;
    ptr2 = std::move(ptr1);

    ptr2->set_test_base(36);

    EXPECT_EQ(ptr2->get_test_base(), 36);
}

TEST_F(UniquePtr2, MayBeMoveAssignedToBaseTypeWithInIndirectPath)
{
    test_unique_ptr2<test_class_with_two_base_with_virtual_base> ptr1;
    ptr1 = make_test_unique2<super_test_class>();
    test_unique_ptr2<test_base_4_with_virtual_base> ptr2;
    ptr2 = std::move(ptr1);

    ptr2->set_test_base_4_with_virtual_base(36);

    EXPECT_EQ(ptr2->get_test_base_4_with_virtual_base(), 36);
}

TEST_F(UniquePtr2, MayBeMoveAssignedToVirtualBase)
{
    test_unique_ptr2<virual_base> ptr1;
    ptr1 = make_test_unique2<super_test_class>();

    ptr1->set_virual_base(34);

    EXPECT_EQ(ptr1->get_virual_base(), 34);
}

TEST_F(UniquePtr2, MakeUniqueCreatesObjectsWithConstLRefParameter)
{
    {
        test_base param;
        make_test_unique2<test_class_with_parameteraized_ctor>(param);
    }

    EXPECT_EQ(life_cycle_tracker::ctor_called, 1);
    EXPECT_EQ(life_cycle_tracker::dtor_called, 2);
    EXPECT_EQ(life_cycle_tracker::copy_called, 1);
    EXPECT_EQ(life_cycle_tracker::move_called, 0);
    EXPECT_EQ(life_cycle_tracker::copy_assign_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_assign_called, 0);
}

TEST_F(UniquePtr2, MakeUniqueCreatesObjectsWithRRefParameter)
{
    {
        test_base param;
        make_test_unique2<test_class_with_parameteraized_ctor>(std::move(param));
    }

    EXPECT_EQ(life_cycle_tracker::ctor_called, 1);
    EXPECT_EQ(life_cycle_tracker::dtor_called, 2);
    EXPECT_EQ(life_cycle_tracker::copy_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_called, 1);
    EXPECT_EQ(life_cycle_tracker::copy_assign_called, 0);
    EXPECT_EQ(life_cycle_tracker::move_assign_called, 0);
}

TEST_F(UniquePtr2, PerformsNoMemoryLeakIfObjectConstructorThrowsException)
{
    EXPECT_ANY_THROW(make_test_unique2<throw_ctor_class>());
}

TEST_F(UniquePtr2, DestroysDerivedObjectWhenHoldingBaseClassPtr)
{
    test_unique_ptr2<test_base_1> ptr(make_test_unique2<super_test_class>());

    ptr.reset();

    ASSERT_EQ(super_test_class::destroyed_times, 1);
}

TEST_F(UniquePtr2, AllowsToConvertRawPointerToDerivedType)
{
    test_unique_ptr2<test_base_1> ptr(make_test_unique2<super_test_class>());

    auto down_casted_ptr = dynamic_cast<super_test_class *>(ptr.get());
    ASSERT_TRUE(down_casted_ptr != nullptr );

    down_casted_ptr->set_super_test_class(34);
    ASSERT_EQ(down_casted_ptr->get_super_test_class(), 34);
}
