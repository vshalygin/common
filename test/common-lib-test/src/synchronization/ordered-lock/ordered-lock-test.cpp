#include <common-lib/synchronization/ordered-lock.h>
#include <gtest/gtest.h>

using namespace vshalygin::cl;
using namespace testing;

namespace {

    std::unique_ptr<std::vector<size_t>> lock_order;
    std::unique_ptr<std::vector<size_t>> unlock_order;

    template<size_t Order>
    class test
    {
    public:
        static constexpr size_t order = Order;

        explicit test(bool throws_on_lock = false)
            : m_throws_on_lock(throws_on_lock)
        {
        }

        void lock()
        {
            if(m_is_locked) {
                throw std::runtime_error("");
            }

            if(m_throws_on_lock) {
                throw std::runtime_error("");
            }

            m_is_locked = true;
            lock_order->push_back(order);
        }

        void unlock()
        {
            if(!m_is_locked) {
                throw std::runtime_error("");
            }

            m_is_locked = false;
            unlock_order->push_back(order);
        }

        bool is_locked() const
        {
            return m_is_locked;
        }

    private:
        bool m_is_locked = false;
        bool m_throws_on_lock = false;
    };
}

class OrderedLock
    : public Test
{
protected:
    void SetUp() override
    {
        lock_order = std::make_unique<std::vector<size_t>>();
        unlock_order = std::make_unique<std::vector<size_t>>();
    }

    void TearDown() override
    {
        ASSERT_EQ(lock_order->size(), unlock_order->size());
        for(size_t i = 0; i < lock_order->size(); ++i) {
            EXPECT_EQ(lock_order->at(i), unlock_order->at(lock_order->size() - i - 1));
        }
        
        lock_order.reset();
        unlock_order.reset();
    }
};

TEST_F(OrderedLock, BasicTest)
{
    test<0> t1;
    test<1> t2;
    test<2> t3;
    test<3> t4;

    auto lock = ordered_lock(t4, t1, t3, t2);
    static_assert(std::is_same_v<decltype(lock),
                  ordered_lock<test<3>, test<0>, test<2>, test<1>>>);

    
    EXPECT_TRUE(t1.is_locked());
    EXPECT_TRUE(t2.is_locked());
    EXPECT_TRUE(t3.is_locked());
    EXPECT_TRUE(t4.is_locked());
    EXPECT_TRUE(lock.is_locked());
    EXPECT_TRUE(lock);
}

TEST_F(OrderedLock, LocksInOrder)
{
    test<0> t1;
    test<1> t2;
    test<2> t3;
    test<3> t4;

    auto lock = ordered_lock(t4, t1, t3, t2);

    ASSERT_EQ((*lock_order).size(), 4);
    EXPECT_EQ((*lock_order)[0], 0);
    EXPECT_EQ((*lock_order)[1], 1);
    EXPECT_EQ((*lock_order)[2], 2);
    EXPECT_EQ((*lock_order)[3], 3);
}

TEST_F(OrderedLock, UnlocksInReverseOrder)
{
    {
        test<0> t1;
        test<1> t2;
        test<2> t3;
        test<3> t4;

        auto lock = ordered_lock(t4, t1, t3, t2);
    }
    
    ASSERT_EQ((*unlock_order).size(), 4);
    EXPECT_EQ((*unlock_order)[0], 3);
    EXPECT_EQ((*unlock_order)[1], 2);
    EXPECT_EQ((*unlock_order)[2], 1);
    EXPECT_EQ((*unlock_order)[3], 0);
}

TEST_F(OrderedLock, DefaultConstructible)
{
    ordered_lock<test<0>, test<1>> l;

    EXPECT_FALSE(l.is_locked());
    EXPECT_FALSE(l);
}

TEST_F(OrderedLock, ConstructFromLocables)
{
    test<0> t1;
    test<1> t2;

    ordered_lock l(t1, t2);

    EXPECT_TRUE(t1.is_locked());
    EXPECT_TRUE(t2.is_locked());
    EXPECT_TRUE(l.is_locked());
    EXPECT_TRUE(l);
}

TEST_F(OrderedLock, ConstructFromAnotherOrderLock)
{
    test<0> t1;
    test<1> t2;

    auto lock1 = ordered_lock(t1, t2);
    ordered_lock<test<0>, test<1>> lock2(std::move(lock1));

    EXPECT_TRUE(t1.is_locked());
    EXPECT_TRUE(t2.is_locked());
    EXPECT_FALSE(lock1.is_locked());
    EXPECT_TRUE(lock2.is_locked());
    EXPECT_FALSE(lock1);
    EXPECT_TRUE(lock2);
}

TEST_F(OrderedLock, ConstructFromAnotherOrderLockWithMessedTemplateParameters)
{
    test<0> t1;
    test<1> t2;

    auto lock1 = ordered_lock(t1, t2);
    ordered_lock<test<1>, test<0>> lock2(std::move(lock1));

    EXPECT_TRUE(t1.is_locked());
    EXPECT_TRUE(t2.is_locked());
    EXPECT_FALSE(lock1.is_locked());
    EXPECT_TRUE(lock2.is_locked());
    EXPECT_FALSE(lock1);
    EXPECT_TRUE(lock2);
}

TEST_F(OrderedLock, ConstructDeferLock)
{
    test<0> t1;
    test<1> t2;

    auto lock = ordered_lock(defer_lock_t{}, t1, t2);

    EXPECT_TRUE(lock);
    EXPECT_FALSE(lock.is_locked());
    EXPECT_FALSE(t1.is_locked());
    EXPECT_FALSE(t1.is_locked());
}

TEST_F(OrderedLock, ConstructAdoptLock)
{
    test<0> t1; t1.lock();
    test<1> t2; t2.lock();

    auto lock = ordered_lock(adopt_lock_t{}, t2, t1);

    EXPECT_TRUE(lock);
    EXPECT_TRUE(lock.is_locked());
    EXPECT_TRUE(t1.is_locked());
    EXPECT_TRUE(t1.is_locked());
}

TEST_F(OrderedLock, AssignAnotherOrderLock)
{
    test<0> t1;
    test<1> t2;
    test<0> t3;
    test<1> t4;

    auto lock1 = ordered_lock(t1, t2);
    auto lock2 = ordered_lock(t3, t4);
    lock2 = std::move(lock1);

    EXPECT_EQ((*unlock_order).size(), 2);
    EXPECT_TRUE(t1.is_locked());
    EXPECT_TRUE(t2.is_locked());
    EXPECT_FALSE(t3.is_locked());
    EXPECT_FALSE(t4.is_locked());
    EXPECT_FALSE(lock1.is_locked());
    EXPECT_TRUE(lock2.is_locked());
    EXPECT_FALSE(lock1);
    EXPECT_TRUE(lock2);
}

TEST_F(OrderedLock, AssignAnotherOrderLockWithMessedTemplateParameters)
{
    test<0> t1;
    test<1> t2;
    test<0> t3;
    test<1> t4;

    auto lock1 = ordered_lock(t1, t2);
    auto lock2 = ordered_lock(t4, t3);
    lock2 = std::move(lock1);

    EXPECT_EQ((*unlock_order).size(), 2);
    EXPECT_TRUE(t1.is_locked());
    EXPECT_TRUE(t2.is_locked());
    EXPECT_FALSE(t3.is_locked());
    EXPECT_FALSE(t4.is_locked());
    EXPECT_FALSE(lock1.is_locked());
    EXPECT_TRUE(lock2.is_locked());
    EXPECT_FALSE(lock1);
    EXPECT_TRUE(lock2);
}

TEST_F(OrderedLock, AssignItselfDoesNothing)
{
    test<0> t1;
    test<1> t2;

    auto lock1 = ordered_lock(t1, t2);
    lock1 = std::move(lock1);

    EXPECT_EQ((*unlock_order).size(), 0);
    EXPECT_TRUE(t1.is_locked());
    EXPECT_TRUE(t2.is_locked());
    EXPECT_TRUE(lock1.is_locked());
    EXPECT_TRUE(lock1);
}

TEST_F(OrderedLock, DoLock)
{
    test<0> t1;
    test<1> t2;

    auto lock = ordered_lock(defer_lock_t{}, t1, t2);
    lock.lock();

    EXPECT_TRUE(t1.is_locked());
    EXPECT_TRUE(t2.is_locked());
    EXPECT_TRUE(lock.is_locked());
    EXPECT_TRUE(lock);
}

TEST_F(OrderedLock, DoUnlock)
{
    test<0> t1;
    test<1> t2;

    auto lock = ordered_lock(t1, t2);
    lock.unlock();

    EXPECT_FALSE(t1.is_locked());
    EXPECT_FALSE(t2.is_locked());
    EXPECT_FALSE(lock.is_locked());
    EXPECT_TRUE(lock);
}

TEST_F(OrderedLock, DoSafeLock)
{
    test<0> t1;
    test<1> t2;
    test<2> t3(true);

    auto lock = ordered_lock(defer_lock_t{}, t1, t2, t3);
    ASSERT_ANY_THROW(lock.lock());

    EXPECT_FALSE(t1.is_locked());
    EXPECT_FALSE(t2.is_locked());
    EXPECT_FALSE(t3.is_locked());
    EXPECT_TRUE(lock);
}

TEST_F(OrderedLock, DoSafeLockOnConstruct)
{
    test<0> t1;
    test<1> t2;
    test<2> t3(true);

    ASSERT_ANY_THROW(ordered_lock(t1, t2, t3));

    EXPECT_FALSE(t1.is_locked());
    EXPECT_FALSE(t2.is_locked());
    EXPECT_FALSE(t3.is_locked());
}

TEST_F(OrderedLock, PushBackNewLockablesInLockState)
{
    test<0> t1;
    test<1> t2;
    test<2> t3;

    auto lock1 = ordered_lock(t1, t2);
    auto lock2 = push_back(std::move(lock1), t3);

    EXPECT_EQ((*unlock_order).size(), 0);
    EXPECT_TRUE(t1.is_locked());
    EXPECT_TRUE(t2.is_locked());
    EXPECT_TRUE(t3.is_locked());
    EXPECT_FALSE(lock1);
    EXPECT_TRUE(lock2);
}

TEST_F(OrderedLock, PushBackNewLockablesInUnlockState)
{
    test<0> t1;
    test<1> t2;
    test<2> t3;

    auto lock1 = ordered_lock(defer_lock_t{}, t1, t2);
    auto lock2 = push_back(std::move(lock1), t3);

    EXPECT_EQ((*unlock_order).size(), 0);
    EXPECT_FALSE(t1.is_locked());
    EXPECT_FALSE(t2.is_locked());
    EXPECT_FALSE(t3.is_locked());
    EXPECT_FALSE(lock1);
    EXPECT_TRUE(lock2);
}
